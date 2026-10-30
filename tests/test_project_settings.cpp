#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/editor/LayerUsage.hpp>
#include <yqlora/editor/ProjectSetup.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/project/GeneralSettings.hpp>
#include <yqlora/project/Project.hpp>
#include <yqlora/scene/ExecutionOrder.hpp>
#include <yqlora/scene/ExecutionOrderSettings.hpp>
#include <yqlora/scene/LayerSettings.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>
#include <yqlora/scene/TimeSettings.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

struct Fixture {
    Fixture()
    {
        const auto location = std::filesystem::temp_directory_path() / "yqlora_tests" / "settings";
        std::filesystem::remove_all(location);
        std::filesystem::create_directories(location);
        std::string error;
        directory = createProject(location, "Game", serializer, error)->directory();
    }

    Project open() const
    {
        return *Project::open(directory, registry);
    }

    const Property& property(std::string_view name) const
    {
        const auto properties = registry.properties(*registry.find<GeneralSettings>());

        return **std::ranges::find(properties, name, &Property::name);
    }

    test::EngineRegistry registry;
    SceneSerializer serializer{registry};
    std::filesystem::path directory;
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "ProjectSettings: every registered section, read from its own file")
{
    Project project = open();
    auto& general = project.settings().get<GeneralSettings>();
    CHECK(general.gameName == "Game");
    CHECK(general.gameVersion == GeneralSettings::defaultGameVersion());

    general.company = "Studio";
    REQUIRE(project.settings().save(general));
    CHECK(std::filesystem::exists(directory / "settings" / "general.json"));
    CHECK(open().settings().get<GeneralSettings>().company == "Studio");

    // Нет файла - значения по умолчанию; файл раздела, которого нет в реестре, не трогается.
    std::filesystem::remove(directory / "settings" / "general.json");
    std::ofstream(directory / "settings" / "physics.json") << R"({"version": 1, "values": {"gravity": 1}})";
    const Project reopened = open();
    CHECK(reopened.settings().get<GeneralSettings>().company.empty());
    CHECK(reopened.settings().find("PhysicsSettings") == nullptr);
    CHECK(std::filesystem::exists(directory / "settings" / "physics.json"));

    // Настройки не ресурсы игры.
    CHECK(reopened.resources().id("settings/physics.json") == 0);
}

TEST_CASE_FIXTURE(Fixture, "SettingsEditing: changes are written at once, undone with the scene history")
{
    Input input;
    EngineServices services{input, registry};
    EditorSession session(services, serializer);
    REQUIRE(session.open(open()));
    auto& general = session.project()->settings().get<GeneralSettings>();

    REQUIRE(session.settingsEditing().setProperty(general, property("company"), PropertyValue{std::string("Studio")}));
    CHECK(general.company == "Studio");
    CHECK(open().settings().get<GeneralSettings>().company == "Studio");
    CHECK_FALSE(session.sceneModified());

    REQUIRE(session.undo());
    CHECK(general.company.empty());
    CHECK(open().settings().get<GeneralSettings>().company.empty());
    REQUIRE(session.redo());
    CHECK(general.company == "Studio");
}

TEST_CASE_FIXTURE(Fixture, "SettingsEditing: the startup scene is chosen with undo")
{
    std::ofstream(directory / "level.yscene") << R"({"version": 1})";
    Input input;
    EngineServices services{input, registry};
    EditorSession session(services, serializer);
    REQUIRE(session.open(open()));
    const auto main = session.project()->startupScenePath();
    session.project()->resources().add("level.yscene");

    REQUIRE(session.settingsEditing().setStartupScene("level.yscene"));
    CHECK(session.project()->startupScenePath() == std::filesystem::path("level.yscene"));
    CHECK(open().startupScenePath() == std::filesystem::path("level.yscene"));
    CHECK_FALSE(session.settingsEditing().setStartupScene("missing.yscene"));

    REQUIRE(session.undo());
    CHECK(session.project()->startupScenePath() == main);
}

TEST_CASE_FIXTURE(Fixture, "TimeSettings: the scene takes the fixed step from the open project")
{
    Input input;
    EngineServices services{input, registry};
    CHECK(timeSettingsOf(services).fixedStep() == kDefaultFixedStep);
    CHECK(timeSettingsOf(services).maxFrame() == kDefaultMaxFrame);

    Project project = open();
    auto& time = project.settings().get<TimeSettings>();
    time.setFixedStep(0.0f);
    CHECK(time.fixedStep() == kMinTimeStep);
    time.setMaxFrame(10.0f);
    CHECK(time.maxFrame() == kMaxTimeStep);
    time.setFixedStep(0.5f);

    services.setProject(&project);
    Scene scene{services};
    scene.advance(0.0f);
    CHECK(scene.time().fixedStep() == 0.5f);

    // Правка во время игры применяется со следующего кадра.
    time.setFixedStep(0.25f);
    scene.advance(0.0f);
    CHECK(scene.time().fixedStep() == 0.25f);
    services.setProject(nullptr);
}

TEST_CASE("LayerSettings: 32 names, layer 0 is always Default")
{
    LayerSettings layers;
    CHECK(layers.names().size() == kLayerCount);
    CHECK(layers.name(0) == kDefaultLayerName);

    layers.setNames({"Renamed", "Player"});
    CHECK(layers.names().size() == kLayerCount);
    CHECK(layers.name(0) == kDefaultLayerName);
    CHECK(layers.name(1) == "Player");
    CHECK(layers.name(2).empty());
    CHECK(layers.name(kLayerCount).empty());
}

TEST_CASE_FIXTURE(Fixture, "LayerUsage: nodes on a layer are found in every scene of the project")
{
    Scene other{EngineServices::none()};
    other.createRoot<Node3D>("Root").addChild<Node3D>("Enemy").setLayer(3);
    REQUIRE(serializer.save(other, directory / "other.yscene"));

    Input input;
    EngineServices services{input, registry};
    EditorSession session(services, serializer);
    REQUIRE(session.open(open()));
    session.project()->resources().add("other.yscene");
    // Открытая сцена считается с несохранёнными правками.
    session.scene().root()->setLayer(3);

    const LayerUsage usage = layerUsage(session, 3);
    CHECK(usage.nodes == 2);
    CHECK(usage.scenes.size() == 2);
    CHECK(layerUsage(session, 4).nodes == 0);

    const std::vector<std::string> before{"Default", "A", "", "B"};
    const std::vector<std::string> after{"Default", "", "", "B"};
    CHECK(clearedLayers(before, after) == std::vector<int>{1});
}

TEST_CASE_FIXTURE(Fixture, "ExecutionOrderSettings: records are saved and override the order from code")
{
    Project project = open();
    auto& order = project.settings().get<ExecutionOrderSettings>();
    order.entries = {{.type = "MeshRenderer", .order = -50}, {.type = "Unloaded", .order = 7}};
    REQUIRE(project.settings().save(order));

    // Записи читаются из файла поле за полем; тип, которого нет в реестре, хранится как есть.
    Project reopened = open();
    const auto& loaded = reopened.settings().get<ExecutionOrderSettings>().entries;
    REQUIRE(loaded.size() == 2);
    CHECK(loaded[0].type == "MeshRenderer");
    CHECK(loaded[0].order == -50);
    CHECK(loaded[1].type == "Unloaded");

    Input input;
    EngineServices services{input, registry};
    const TypeInfo& renderer = *registry.find("MeshRenderer");
    const TypeInfo& transform = *registry.find("Transform3D");
    CHECK(executionOrderOf(services, renderer) == renderer.executionOrder());
    services.setProject(&reopened);
    CHECK(executionOrderOf(services, renderer) == -50);
    CHECK(executionOrderOf(services, transform) == transform.executionOrder());
    services.setProject(nullptr);
}

TEST_CASE_FIXTURE(Fixture, "Property: a list of records knows its fields and checks them")
{
    const auto properties = registry.properties(*registry.find<ExecutionOrderSettings>());
    const Property& entries = *properties.front();
    CHECK(entries.type() == PropertyType::List);
    CHECK(entries.elementType() == PropertyType::Record);
    REQUIRE(entries.fields().size() == 2);
    CHECK(entries.fields()[0].name() == "type");
    CHECK(entries.fields()[0].hint() == PropertyHint::ComponentType);
    CHECK(entries.fields()[1].type() == PropertyType::Int);
    CHECK(entries.defaultElement() == PropertyValue{RecordValue{{PropertyValue{std::string()}, PropertyValue{0}}}});

    ExecutionOrderSettings section;
    const RecordValue good{{PropertyValue{std::string("A")}, PropertyValue{3}}};
    const RecordValue wrongField{{PropertyValue{std::string("A")}, PropertyValue{3.0f}}};
    const RecordValue missingField{{PropertyValue{std::string("A")}}};
    CHECK_FALSE(entries.set(section, ListValue{{PropertyValue{wrongField}}}));
    CHECK_FALSE(entries.set(section, ListValue{{PropertyValue{missingField}}}));
    REQUIRE(entries.set(section, ListValue{{PropertyValue{good}}}));
    CHECK(section.orderOf("A") == 3);
    CHECK(section.orderOf("B") == std::nullopt);
}
