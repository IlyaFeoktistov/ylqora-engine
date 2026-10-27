#include <doctest/doctest.h>

#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/editor/Project.hpp>
#include <yqlora/editor/ProjectFiles.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <filesystem>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

struct Fixture {
    Fixture()
    {
        registerEngineTypes(registry);
        const auto location = std::filesystem::temp_directory_path() / "yqlora_tests" / "files";
        std::filesystem::remove_all(location);
        std::filesystem::create_directories(location);
        std::string error;
        project = *Project::create(location, "Game", serializer, error);
    }

    TypeRegistry registry;
    SceneSerializer serializer{registry};
    Project project;
};

std::vector<std::string> names(const projectfiles::Entry& entry)
{
    std::vector<std::string> result;
    for (const auto& child : entry.children) {
        result.push_back(child.name);
    }

    return result;
}

} // namespace

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: folders first, the project file is hidden")
{
    REQUIRE(projectfiles::createFolder(project, {}));
    REQUIRE(projectfiles::createScene(project, {}, serializer));

    const auto root = projectfiles::scan(project);
    CHECK(names(root) == std::vector<std::string>{"New Folder", "New Scene.yscene", "main.yscene"});
    CHECK(root.children[0].directory);
}

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: new scenes get free names and can be opened")
{
    const auto first = projectfiles::createScene(project, {}, serializer);
    const auto second = projectfiles::createScene(project, {}, serializer);
    REQUIRE(first);
    REQUIRE(second);
    CHECK(*first == "New Scene.yscene");
    CHECK(*second == "New Scene 2.yscene");

    Scene scene;
    EditorSession session(scene, serializer);
    REQUIRE(session.open(project));
    REQUIRE(session.openScene(*second));
    CHECK(session.scenePath() == *second);
    CHECK(scene.empty());

    // Сохраняется в открытую сцену, а не в главную.
    session.createNode(*registry.find("Node3D"), nullptr);
    REQUIRE(session.saveScene());
    Scene check;
    REQUIRE(serializer.load(check, project.directory / *second));
    CHECK_FALSE(check.empty());
}

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: nothing outside the project can be touched")
{
    CHECK_FALSE(projectfiles::canModify(project, ""));
    CHECK_FALSE(projectfiles::canModify(project, "project.yqlora"));
    CHECK_FALSE(projectfiles::canModify(project, "../other"));
    CHECK_FALSE(projectfiles::canModify(project, "a/../../other"));
    CHECK(projectfiles::canModify(project, "main.yscene"));

    CHECK_FALSE(projectfiles::remove(project, "../files"));
    CHECK(std::filesystem::exists(project.directory));
    CHECK_FALSE(projectfiles::rename(project, "main.yscene", "bad/name.yscene"));
}

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: renaming the open main scene keeps the session and project in sync")
{
    Scene scene;
    EditorSession session(scene, serializer);
    REQUIRE(session.open(project));

    const auto folder = projectfiles::createFolder(project, {});
    REQUIRE(folder);
    const auto moved = projectfiles::rename(project, "main.yscene", "level.yscene");
    REQUIRE(moved);
    session.fileRenamed("main.yscene", *moved);
    CHECK(session.scenePath() == "level.yscene");
    CHECK(session.project()->mainScene == "level.yscene");
    CHECK(Project::load(project.directory)->mainScene == "level.yscene");

    CHECK(projectfiles::remove(project, *folder));
    CHECK_FALSE(std::filesystem::exists(project.directory / *folder));
}
