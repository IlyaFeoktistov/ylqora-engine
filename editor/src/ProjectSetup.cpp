#include "yqlora/editor/ProjectSetup.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/editor/ProjectFiles.hpp"
#include "yqlora/resource/BuiltinResources.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/MeshRenderer.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

namespace yqlora::editor {

namespace {

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.generic_u8string();

    return {text.begin(), text.end()};
}

std::filesystem::path fromUtf8(const std::string& text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

// Чтобы новый проект сразу что-то показывал: камера и куб на сетке.
void buildStarterScene(Scene& scene)
{
    auto& root = scene.createRoot<Node3D>("Main");
    auto& camera = root.addChild<Camera3D>("Camera");
    camera.transform().setPosition({0.0f, 2.0f, 5.0f});
    camera.transform().lookAt({0.0f, 0.5f, 0.0f});
    scene.setActiveCamera(&camera);

    auto& cube = root.addChild<Node3D>("Cube");
    cube.transform().setPosition({0.0f, 0.5f, 0.0f});
    cube.addComponent<MeshRenderer>().setMesh(ResourceRef{builtin::kCubeMesh});
}

} // namespace

std::optional<Project> createProject(
    const std::filesystem::path& location,
    const std::string& name,
    const SceneSerializer& serializer,
    std::string& error
)
{
    if (!isValidFileName(name)) {
        error = "projects.error.invalidName";

        return std::nullopt;
    }
    const std::filesystem::path directory = location / fromUtf8(name);
    std::error_code ec;
    if (std::filesystem::exists(directory, ec)) {
        error = "projects.error.exists";

        return std::nullopt;
    }
    if (!std::filesystem::create_directories(directory, ec)) {
        error = "projects.error.cannotCreate";

        return std::nullopt;
    }

    const std::filesystem::path mainScene = std::string("main") + std::string(ProjectFiles::sceneExtension());
    Scene scene;
    buildStarterScene(scene);
    auto project = Project::create(directory, name);
    if (!project || !serializer.save(scene, directory / mainScene)) {
        error = "projects.error.cannotCreate";

        return std::nullopt;
    }
    project->setMainScene(project->resources().add(mainScene));
    if (!project->save()) {
        error = "projects.error.cannotCreate";

        return std::nullopt;
    }

    return project;
}

bool deleteProjectFolder(const std::filesystem::path& directory, std::string& error)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(directory / Project::fileName(), ec)) {
        error = "projects.error.notProject";

        return false;
    }
    std::filesystem::remove_all(directory, ec);
    if (ec) {
        log::error("Cannot delete project folder '{}': {}", toUtf8(directory), ec.message());
        error = "projects.error.cannotDelete";

        return false;
    }

    return true;
}

bool isValidFileName(std::string_view name)
{
    if (name.empty() || name.size() > 64 || name == "." || name == "..") {
        return false;
    }
    // Пробел или точка в конце: Windows их молча отбрасывает у имён папок.
    if (name.front() == ' ' || name.back() == ' ' || name.back() == '.') {
        return false;
    }
    for (const char c : name) {
        if (static_cast<unsigned char>(c) < 32 || std::string_view("/\\:*?\"<>|").find(c) != std::string_view::npos) {
            return false;
        }
    }

    return true;
}

} // namespace yqlora::editor
