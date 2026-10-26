#include <doctest/doctest.h>

#include <yqlora/editor/Project.hpp>
#include <yqlora/editor/ProjectList.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <filesystem>

using namespace yqlora;

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
    const auto project = Project::create(location, "My Game", serializer, error);
    REQUIRE(project);
    CHECK(project->directory == location / "My Game");
    CHECK(std::filesystem::exists(project->projectFile()));

    const auto loaded = Project::load(project->directory);
    REQUIRE(loaded);
    CHECK(loaded->name == "My Game");

    Scene scene;
    REQUIRE(serializer.load(scene, loaded->mainScenePath()));
    REQUIRE(scene.root());
    CHECK(scene.root()->children().size() == 2);
    CHECK(scene.activeCamera() != nullptr);

    // Второй раз в ту же папку нельзя.
    CHECK_FALSE(Project::create(location, "My Game", serializer, error));
    CHECK(error == "projects.error.exists");
}

TEST_CASE("Project: names must be usable as folder names everywhere")
{
    CHECK(Project::isValidName("Game"));
    CHECK(Project::isValidName("Моя игра 2"));

    CHECK_FALSE(Project::isValidName(""));
    CHECK_FALSE(Project::isValidName(".."));
    CHECK_FALSE(Project::isValidName("a/b"));
    CHECK_FALSE(Project::isValidName("what?"));
    CHECK_FALSE(Project::isValidName(" padded"));
    CHECK_FALSE(Project::isValidName("dot."));
}

TEST_CASE("Project: a folder without a project file is not a project")
{
    CHECK_FALSE(Project::load(freshDirectory("project_missing")));
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
