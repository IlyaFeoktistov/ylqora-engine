#include "yqlora/editor/Project.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/MeshInstance3D.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace yqlora::editor {

namespace {

using Json = nlohmann::json;

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

    auto& cube = root.addChild<MeshInstance3D>("Cube");
    cube.transform().setPosition({0.0f, 0.5f, 0.0f});
}

} // namespace

std::optional<Project> Project::load(const std::filesystem::path& directory)
{
    std::ifstream in(directory / kFileName);
    if (!in) {
        return std::nullopt;
    }
    const Json json = Json::parse(in, nullptr, /*allow_exceptions=*/false);
    if (!json.is_object() || !json.contains("name") || !json["name"].is_string()) {
        log::warn("Project file in '{}' is broken", toUtf8(directory));

        return std::nullopt;
    }

    Project project;
    project.name = json["name"].get<std::string>();
    project.directory = directory;
    if (const auto it = json.find("mainScene"); it != json.end() && it->is_string()) {
        project.mainScene = fromUtf8(it->get<std::string>());
    }

    return project;
}

bool Project::save() const
{
    Json json;
    json["name"] = name;
    json["mainScene"] = toUtf8(mainScene);

    std::ofstream out(projectFile());
    out << json.dump(4) << '\n';

    return static_cast<bool>(out);
}

std::optional<Project> Project::create(
    const std::filesystem::path& location,
    const std::string& name,
    const SceneSerializer& serializer,
    std::string& error
)
{
    if (!isValidName(name)) {
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

    Project project;
    project.name = name;
    project.directory = directory;

    Scene scene;
    buildStarterScene(scene);
    if (!project.save() || !serializer.save(scene, project.mainScenePath())) {
        error = "projects.error.cannotCreate";

        return std::nullopt;
    }

    return project;
}

bool Project::deleteFolder(const std::filesystem::path& directory, std::string& error)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(directory / kFileName, ec)) {
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

bool Project::isValidName(std::string_view name)
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
