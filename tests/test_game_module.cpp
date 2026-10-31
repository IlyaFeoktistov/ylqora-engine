#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scripting/GameModule.hpp>

#include <algorithm>
#include <array>
#include <filesystem>

using namespace yqlora;

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
