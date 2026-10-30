#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/core/EngineTypes.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/editor/ProjectFiles.hpp>
#include <yqlora/editor/ProjectSetup.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/project/Project.hpp>
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
        project = *createProject(location, "Game", serializer, error);
    }

    TypeRegistry registry;
    SceneSerializer serializer{registry};
    Project project;
    ProjectFiles files{project};
};

std::vector<std::string> names(const ProjectFiles::Entry& entry)
{
    std::vector<std::string> result;
    for (const auto& child : entry.children) {
        result.push_back(child.name);
    }

    return result;
}

} // namespace

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: folders first, the project file and .meta are hidden")
{
    REQUIRE(files.createFolder({}));
    REQUIRE(files.createScene({}, serializer));

    const auto root = files.scan();
    CHECK(names(root) == std::vector<std::string>{"New Folder", "New Scene.yscene", "main.yscene"});
    CHECK(root.children[0].directory);
}

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: new scenes get free names and can be opened")
{
    const auto first = files.createScene({}, serializer);
    const auto second = files.createScene({}, serializer);
    REQUIRE(first);
    REQUIRE(second);
    CHECK(*first == "New Scene.yscene");
    CHECK(*second == "New Scene 2.yscene");

    Input input;

    EngineServices services{input, registry};

    EditorSession session(services, serializer);
    Scene& scene = session.scene();
    REQUIRE(session.open(project));
    REQUIRE(session.openScene(*second));
    CHECK(session.document().id() == project.resources().id(*second));
    CHECK(session.scenePath() == *second);
    CHECK(scene.empty());

    // Сохраняется в открытую сцену, а не в главную.
    session.editing().createNode(*registry.find("Node3D"), nullptr);
    REQUIRE(session.saveScene());
    Scene check{EngineServices::none()};
    REQUIRE(serializer.load(check, project.directory() / *second));
    CHECK_FALSE(check.empty());
}

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: nothing outside the project can be touched")
{
    CHECK_FALSE(files.canModify(""));
    CHECK_FALSE(files.canModify("project.yqlora"));
    CHECK_FALSE(files.canModify("../other"));
    CHECK_FALSE(files.canModify("a/../../other"));
    CHECK(files.canModify("main.yscene"));

    CHECK_FALSE(files.remove("../files"));
    CHECK(std::filesystem::exists(project.directory()));
    CHECK_FALSE(files.rename("main.yscene", "bad/name.yscene"));
}

TEST_CASE_FIXTURE(Fixture, "ProjectFiles: a renamed scene keeps its id, the session and project follow it")
{
    Input input;
    EngineServices services{input, registry};
    EditorSession session(services, serializer);
    REQUIRE(session.open(project));
    const ResourceId id = session.document().id();
    REQUIRE(id != 0);

    // Сессия работает со своим экземпляром проекта: файлы меняем через него.
    ProjectFiles sessionFiles(*session.project());
    const auto folder = sessionFiles.createFolder({});
    REQUIRE(folder);
    const auto moved = sessionFiles.rename("main.yscene", "level.yscene");
    REQUIRE(moved);
    CHECK(session.document().id() == id);
    CHECK(session.scenePath() == "level.yscene");
    CHECK(session.project()->mainScenePath() == std::filesystem::path("level.yscene"));
    CHECK(std::filesystem::exists(project.directory() / "level.yscene.meta"));
    CHECK(Project::open(project.directory())->mainScenePath() == std::filesystem::path("level.yscene"));

    CHECK(sessionFiles.remove(*folder));
    CHECK_FALSE(std::filesystem::exists(project.directory() / *folder));
}
