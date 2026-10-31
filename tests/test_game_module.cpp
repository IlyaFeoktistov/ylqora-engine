#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/editor/Inspection.hpp>
#include <yqlora/editor/ProjectSetup.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/project/Project.hpp>
#include <yqlora/scene/MissingComponent.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>
#include <yqlora/scripting/GameModule.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

constexpr std::string_view kScript = "TestMover";

struct Fixture {
    // Скрипт модуля на корне сцены, как его добавляет редактор: по имени типа.
    Component& addScript(Scene& scene, std::string name) const
    {
        auto& root = scene.createRoot<Node3D>(std::move(name));

        return root.addComponent(registry.find(kScript)->createComponent());
    }

    const Property& property(std::string_view name) const
    {
        const auto properties = registry.properties(*registry.find(kScript));

        return **std::ranges::find(properties, name, &Property::name);
    }

    test::EngineRegistry registry;
    Input input;
    EngineServices services{input, registry};
    GameModule module{registry};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "GameModule: a missing library is not loaded")
{
    CHECK_FALSE(module.load(std::filesystem::path(YQLORA_TEST_GAME_MODULE).parent_path() / "missing-game-module"));
    CHECK_FALSE(module.loaded());
}

TEST_CASE_FIXTURE(Fixture, "GameModule: load, register, create, run, destroy, unload and load again")
{
    REQUIRE(module.load(YQLORA_TEST_GAME_MODULE));
    CHECK(module.rejected().empty());
    const TypeInfo* type = registry.find(kScript);
    REQUIRE(type);
    CHECK(type->module() == GameModule::moduleName());
    CHECK(type->isScript());

    // Сцена редактора и сцена Play, в обеих экземпляры типа из библиотеки.
    Scene edited{services};
    Scene played{services};
    Component& editedScript = addScript(edited, "Edited");
    Component& playedScript = addScript(played, "Played");
    CHECK(property("speed").set(editedScript, PropertyValue{3.0f}));
    played.advance(0.1f);
    played.advance(0.1f);
    CHECK(property("updates").get(playedScript) == PropertyValue{2});
    CHECK(played.root()->name() == "Played+start");

    const std::array<Scene*, 2> scenes{&edited, &played};
    module.unload(scenes);
    CHECK_FALSE(module.loaded());
    CHECK(registry.find(kScript) == nullptr);
    CHECK(edited.componentsOfType(kScript).empty());
    CHECK(played.componentsOfType(kScript).empty());
    // onStop пришёл, пока код был загружен; незапущенный скрипт его не получает.
    CHECK(played.root()->name() == "Played+start+stop");
    CHECK(edited.root()->name() == "Edited");

    // Как после пересборки: тот же модуль загружается снова.
    REQUIRE(module.load(YQLORA_TEST_GAME_MODULE));
    CHECK(registry.find(kScript));
    addScript(edited, "Again");
    module.unload(scenes);
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: the game library of the project loads on open and unloads on close")
{
    const auto location = std::filesystem::temp_directory_path() / "yqlora_tests" / "session_game_module";
    std::filesystem::remove_all(location);
    SceneSerializer serializer{registry};
    std::string error;
    const auto created = createProject(location, "Game", serializer, error);
    REQUIRE(created);
    const auto library = gameLibraryPath(created->directory());
    std::filesystem::create_directories(library.parent_path());
    std::filesystem::copy_file(YQLORA_TEST_GAME_MODULE, library);

    EditorSession session{services, serializer, module};
    REQUIRE(session.open(*Project::open(created->directory(), registry)));
    REQUIRE(module.loaded());
    const TypeInfo* type = registry.find(kScript);
    REQUIRE(type);

    // Скрипты - своей группой в "Добавить компонент".
    Node& root = *session.scene().root();
    session.selection().select(root.id());
    const auto inspection = inspect(session);
    REQUIRE(inspection);
    CHECK(std::ranges::find(inspection->addableScripts, type) != inspection->addableScripts.end());
    CHECK(std::ranges::find(inspection->addable, type) == inspection->addable.end());

    // Один скрипт остаётся в сцене, второй снят и лежит в истории правок:
    // оба уничтожаются до выгрузки библиотеки.
    REQUIRE(session.editing().addComponent(root, *type));
    Component* removed = session.editing().addComponent(root, *type);
    REQUIRE(removed);
    REQUIRE(session.editing().removeComponent(root, *removed));
    REQUIRE(session.save());
    session.close();
    CHECK_FALSE(module.loaded());
    CHECK(registry.find(kScript) == nullptr);

    // Снова открытый проект: скрипт из файла настоящий, не заглушка.
    REQUIRE(session.open(*Project::open(created->directory(), registry)));
    CHECK(session.scene().componentsOfType(kScript).size() == 1);

    // Без библиотеки проект открывается, скрипт ждёт её заглушкой.
    session.close();
    std::filesystem::remove(library);
    REQUIRE(session.open(*Project::open(created->directory(), registry)));
    CHECK_FALSE(module.loaded());
    CHECK(std::ranges::any_of(session.scene().root()->components(), [](const auto& component) {
        return dynamic_cast<const MissingComponent*>(component.get()) != nullptr;
    }));
}
