#include "yqlora/project/Project.hpp"

#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Log.hpp"

#include "core/JsonId.hpp"

namespace yqlora {

namespace {

using Json = nlohmann::json;

constexpr int kFormatVersion = 1;

// Ключи файла проекта.
constexpr const char* kVersionKey = "version";
constexpr const char* kNameKey = "name";
constexpr const char* kMainSceneKey = "mainScene";

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.generic_u8string();

    return {text.begin(), text.end()};
}

std::optional<Json> readFile(const std::filesystem::path& directory)
{
    const std::filesystem::path file = directory / Project::fileName();
    if (!std::filesystem::exists(file)) {
        return std::nullopt;
    }
    auto json = readJsonFile(file);
    if (!json || !json->is_object() || !json->contains(kNameKey) || !(*json)[kNameKey].is_string()) {
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
    project.m_name = (*json)[kNameKey].get<std::string>();
    project.m_directory = directory;
    project.m_resources.scan(directory, fileName());

    if (json->value(kVersionKey, 0) != kFormatVersion) {
        log::warn("Project file in '{}' has an unsupported version", toUtf8(directory));

        return std::nullopt;
    }
    if (const auto mainScene = json->find(kMainSceneKey); mainScene != json->end()) {
        project.m_mainScene = idFromJson(*mainScene).value_or(0);
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

    return (*json)[kNameKey].get<std::string>();
}

bool Project::save() const
{
    Json json;
    json[kVersionKey] = kFormatVersion;
    json[kNameKey] = m_name;
    json[kMainSceneKey] = idToJson(m_mainScene);

    return writeJsonFile(projectFile(), json);
}

std::optional<std::filesystem::path> Project::mainScenePath() const
{
    return m_resources.path(m_mainScene);
}

} // namespace yqlora
