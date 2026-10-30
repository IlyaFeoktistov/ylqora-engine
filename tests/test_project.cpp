#include <doctest/doctest.h>

#include <yqlora/core/EngineTypes.hpp>
#include <yqlora/core/Version.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/editor/ProjectList.hpp>
#include <yqlora/editor/ProjectSetup.hpp>
#include <yqlora/project/Project.hpp>
#include <yqlora/project/ProjectUpgrade.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/render/Camera3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <filesystem>
#include <fstream>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

// Своя пустая папка на каждый тест, чтобы тесты не мешали друг другу.
std::filesystem::path freshDirectory(const char* name)
{
    const auto directory = std::filesystem::temp_directory_path() / "yqlora_tests" / name;
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    return directory;
}

} // namespace

TEST_CASE("Project: create makes a folder with the project file and a starter scene")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);
    const auto location = freshDirectory("project_create");

    std::string error;
    const auto project = createProject(location, "My Game", serializer, error);
    REQUIRE(project);
    CHECK(project->directory() == location / "My Game");
    CHECK(std::filesystem::exists(project->projectFile()));

    const auto loaded = Project::open(project->directory());
    REQUIRE(loaded);
    CHECK(loaded->name() == "My Game");
    CHECK(loaded->mainScene() == project->mainScene());
    REQUIRE(loaded->mainScenePath());

    Scene scene{EngineServices::none()};
    REQUIRE(serializer.load(scene, loaded->directory() / *loaded->mainScenePath()));
    REQUIRE(scene.root());
    CHECK(scene.root()->children().size() == 2);
    CHECK(dynamic_cast<Camera3D*>(scene.root()->findChild("Camera")) != nullptr);

    // Второй раз в ту же папку нельзя.
    CHECK_FALSE(createProject(location, "My Game", serializer, error));
    CHECK(error == "projects.error.exists");
}

TEST_CASE("isValidFileName: names must be usable as file and folder names everywhere")
{
    CHECK(isValidFileName("Game"));
    CHECK(isValidFileName("Моя игра 2"));

    CHECK_FALSE(isValidFileName(""));
    CHECK_FALSE(isValidFileName(".."));
    CHECK_FALSE(isValidFileName("a/b"));
    CHECK_FALSE(isValidFileName("what?"));
    CHECK_FALSE(isValidFileName(" padded"));
    CHECK_FALSE(isValidFileName("dot."));
}

TEST_CASE("Project: a folder without a project file is not a project")
{
    CHECK_FALSE(Project::open(freshDirectory("project_missing")));
    CHECK_FALSE(Project::readInfo(freshDirectory("project_missing")));
}

TEST_CASE("ProjectList: recently opened first, saved between runs")
{
    const auto file = freshDirectory("project_list") / "projects.json";

    ProjectList list(file);
    list.add("/games/a");
    list.add("/games/b");
    list.add("/games/b/"); // тот же проект
    REQUIRE(list.entries().size() == 2);

    list.markOpened("/games/b");
    list.markOpened("/games/a");
    CHECK(list.entries()[0].directory == "/games/a");
    CHECK(list.entries()[1].directory == "/games/b");
    REQUIRE(list.save());

    ProjectList reloaded(file);
    reloaded.load();
    REQUIRE(reloaded.entries().size() == 2);
    CHECK(reloaded.entries()[0].directory == "/games/a");

    reloaded.remove("/games/a");
    CHECK(reloaded.entries().size() == 1);
}

TEST_CASE("ProjectList: missing file means an empty list")
{
    ProjectList list(freshDirectory("project_list_missing") / "projects.json");
    list.load();
    CHECK(list.entries().empty());
}

TEST_CASE("EditorSession: open, change, save and reopen a project")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);
    std::string error;
    const auto project = createProject(freshDirectory("session"), "Game", serializer, error);
    REQUIRE(project);

    EditorSession session(EngineServices::none(), serializer);
    Scene& scene = session.scene();
    CHECK_FALSE(session.saveScene()); // проект ещё не открыт

    REQUIRE(session.open(*project));
    REQUIRE(scene.root());
    session.selection().select(scene.root()->id());
    scene.root()->setName("Renamed");
    REQUIRE(session.saveScene());

    session.close();
    CHECK(scene.empty());
    CHECK_FALSE(session.project());
    CHECK(session.selection().selected() == 0);

    REQUIRE(session.open(*project));
    CHECK(scene.root()->name() == "Renamed");
}

TEST_CASE("EditorSession: a project with a broken scene opens with an empty scene")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);
    std::string error;
    const auto project = createProject(freshDirectory("session_broken"), "Game", serializer, error);
    REQUIRE(project);
    std::filesystem::remove(project->directory() / *project->mainScenePath());

    EditorSession session(EngineServices::none(), serializer);
    Scene& scene = session.scene();
    scene.createRoot<Node>("FromBefore");
    CHECK_FALSE(session.open(*project));
    CHECK(session.project());
    CHECK(scene.empty());
}

TEST_CASE("deleteProjectFolder: removes only real project folders")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);
    const auto location = freshDirectory("project_delete");
    std::string error;
    const auto project = createProject(location, "Game", serializer, error);
    REQUIRE(project);

    const auto notProject = location / "Photos";
    std::filesystem::create_directories(notProject);
    CHECK_FALSE(deleteProjectFolder(notProject, error));
    CHECK(error == "projects.error.notProject");
    CHECK(std::filesystem::exists(notProject));

    CHECK(deleteProjectFolder(project->directory(), error));
    CHECK_FALSE(std::filesystem::exists(project->directory()));
}

TEST_CASE("Project: the version tells whether a project opens, needs an upgrade or is too new")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);
    std::string error;
    const auto project = createProject(freshDirectory("project_version"), "Game", serializer, error);
    REQUIRE(project);

    const auto info = Project::readInfo(project->directory());
    REQUIRE(info);
    CHECK(info->name == "Game");
    CHECK(info->formatVersion == Project::formatVersion());
    CHECK(info->engineVersion == engineVersion());
    CHECK(info->compatibility == FormatCompatibility::Current);

    const auto rewrite = [&](const char* text) {
        std::ofstream(project->projectFile()) << text;
    };
    rewrite(R"({"name": "Game", "version": 99, "engine": "9.9.9"})");
    CHECK(Project::readInfo(project->directory())->compatibility == FormatCompatibility::Newer);
    CHECK(Project::readInfo(project->directory())->engineVersion == "9.9.9");
    CHECK_FALSE(Project::open(project->directory()));
    CHECK_FALSE(upgradeProject(project->directory(), serializer));

    // Без версии - файл не этого формата, переводить не с чего.
    rewrite(R"({"name": "Game"})");
    CHECK(Project::readInfo(project->directory())->compatibility == FormatCompatibility::Unsupported);
    CHECK_FALSE(Project::open(project->directory()));
}

TEST_CASE("Project: upgrading a current project rewrites it and keeps it working")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);
    std::string error;
    const auto project = createProject(freshDirectory("project_upgrade"), "Game", serializer, error);
    REQUIRE(project);

    REQUIRE(upgradeProject(project->directory(), serializer));
    const auto reopened = Project::open(project->directory());
    REQUIRE(reopened);
    CHECK(reopened->mainScene() == project->mainScene());

    // Испорченная сцена: перевод отказывается и ничего не трогает.
    std::ofstream(project->directory() / "broken.yscene") << "not json";
    CHECK_FALSE(upgradeProject(project->directory(), serializer));
    CHECK(Project::open(project->directory()));
}
