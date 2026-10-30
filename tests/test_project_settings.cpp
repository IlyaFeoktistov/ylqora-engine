#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/editor/ProjectSetup.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/project/GeneralSettings.hpp>
#include <yqlora/project/Project.hpp>
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
