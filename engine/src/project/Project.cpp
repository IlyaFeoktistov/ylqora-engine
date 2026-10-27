#include "yqlora/project/Project.hpp"

#include "yqlora/core/Log.hpp"

#include "core/JsonId.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace yqlora {

namespace {

using Json = nlohmann::json;

// 1 - главная сцена записана путём, 2 - ID ресурса.
constexpr int kFormatVersion = 2;

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.generic_u8string();

    return {text.begin(), text.end()};
}

std::filesystem::path fromUtf8(const std::string& text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

std::optional<Json> readFile(const std::filesystem::path& directory)
{
    std::ifstream in(directory / Project::fileName());
    if (!in) {
        return std::nullopt;
    }
    Json json = Json::parse(in, nullptr, /*allow_exceptions=*/false);
    if (!json.is_object() || !json.contains("name") || !json["name"].is_string()) {
        log::warn("Project file in '{}' is broken", toUtf8(directory));

        return std::nullopt;
    }

    return json;
}

} // namespace

std::optional<Project> Project::open(const std::filesystem::path& directory)
{
    const auto json = readFile(directory);
    if (!json) {
        return std::nullopt;
    }

    Project project;
    project.m_name = (*json)["name"].get<std::string>();
    project.m_directory = directory;
    project.m_resources.scan(directory, fileName());

    const int version = json->value("version", 1);
    const auto mainScene = json->find("mainScene");
    if (mainScene != json->end() && version >= 2) {
        project.m_mainScene = idFromJson(*mainScene).value_or(0);
    } else if (mainScene != json->end() && mainScene->is_string()) {
        // Версия 1: путь. Теперь у сцены есть ID, переписываем файл в новом виде.
        project.m_mainScene = project.m_resources.id(fromUtf8(mainScene->get<std::string>()));
        if (!project.save()) {
            log::warn("Cannot update the project file in '{}'", toUtf8(directory));
        }
    }

    return project;
}

std::optional<Project> Project::create(const std::filesystem::path& directory, std::string name)
{
    Project project;
    project.m_name = std::move(name);
    project.m_directory = directory;
    if (!project.save()) {
        return std::nullopt;
    }
    project.m_resources.scan(directory, fileName());

    return project;
}

std::optional<std::string> Project::readName(const std::filesystem::path& directory)
{
    const auto json = readFile(directory);
    if (!json) {
        return std::nullopt;
    }

    return (*json)["name"].get<std::string>();
}

bool Project::save() const
{
    Json json;
    json["version"] = kFormatVersion;
    json["name"] = m_name;
    json["mainScene"] = idToJson(m_mainScene);

    std::ofstream out(projectFile());
    out << json.dump(4) << '\n';

    return static_cast<bool>(out);
}

std::optional<std::filesystem::path> Project::mainScenePath() const
{
    return m_resources.path(m_mainScene);
}

} // namespace yqlora
