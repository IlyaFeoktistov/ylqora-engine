#include "yqlora/project/Project.hpp"

#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Version.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include "core/JsonId.hpp"

#include <array>
#include <utility>
#include <vector>

namespace yqlora {

namespace {

using Json = nlohmann::json;

constexpr int kFirstFormatVersion = 1;
constexpr int kFormatVersion = 1;

// kMigrations[i] переводит файл проекта версии i + 1 в i + 2. Сцены переводит
// их собственная цепочка в SceneSerializer. Пока формат один, переводов нет.
using Migration = void (*)(nlohmann::json& project);
constexpr std::array<Migration, kFormatVersion - kFirstFormatVersion> kMigrations{};

// Ключи файла проекта.
constexpr const char* kVersionKey = "version";
constexpr const char* kNameKey = "name";
constexpr const char* kMainSceneKey = "mainScene";
constexpr const char* kEngineVersionKey = "engine";

ProjectCompatibility compatibilityOf(int version)
{
    if (version == kFormatVersion) {
        return ProjectCompatibility::Current;
    }
    if (version > kFormatVersion) {
        return ProjectCompatibility::Newer;
    }

    return version >= kFirstFormatVersion ? ProjectCompatibility::Older : ProjectCompatibility::Unsupported;
}

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

int Project::formatVersion()
{
    return kFormatVersion;
}

std::optional<Project> Project::open(const std::filesystem::path& directory)
{
    const auto json = readFile(directory);
    if (!json) {
        return std::nullopt;
    }
    if (compatibilityOf(json->value(kVersionKey, 0)) != ProjectCompatibility::Current) {
        log::warn("Project in '{}' is not in the current format, upgrade it first", toUtf8(directory));

        return std::nullopt;
    }

    Project project;
    project.m_name = (*json)[kNameKey].get<std::string>();
    project.m_directory = directory;
    project.m_resources.scan(directory, fileName());
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

std::optional<ProjectInfo> Project::readInfo(const std::filesystem::path& directory)
{
    const auto json = readFile(directory);
    if (!json) {
        return std::nullopt;
    }
    ProjectInfo info;
    info.name = (*json)[kNameKey].get<std::string>();
    info.formatVersion = json->value(kVersionKey, 0);
    info.engineVersion = json->value(kEngineVersionKey, "");
    info.compatibility = compatibilityOf(info.formatVersion);

    return info;
}

bool Project::upgrade(const std::filesystem::path& directory, const SceneSerializer& serializer)
{
    auto json = readFile(directory);
    if (!json) {
        return false;
    }
    const int version = json->value(kVersionKey, 0);
    const ProjectCompatibility compatibility = compatibilityOf(version);
    if (compatibility != ProjectCompatibility::Older && compatibility != ProjectCompatibility::Current) {
        return false;
    }

    // Чтение переводит сцену на текущий формат, запись сохраняет уже в нём.
    std::vector<std::pair<std::filesystem::path, Scene>> scenes;
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(directory, ec);
         !ec && it != std::filesystem::recursive_directory_iterator();
         it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == SceneSerializer::fileExtension()) {
            Scene& scene = scenes.emplace_back(it->path(), Scene{}).second;
            if (!serializer.load(scene, it->path())) {
                log::error("Project upgrade: scene '{}' is not read, nothing is changed", toUtf8(it->path()));

                return false;
            }
        }
    }
    for (const auto& [path, scene] : scenes) {
        if (!serializer.save(scene, path)) {
            log::error("Project upgrade: cannot write scene '{}'", toUtf8(path));

            return false;
        }
    }

    for (int from = version; from < kFormatVersion; ++from) {
        kMigrations[static_cast<std::size_t>(from - kFirstFormatVersion)](*json);
    }
    (*json)[kVersionKey] = kFormatVersion;
    (*json)[kEngineVersionKey] = engineVersion();

    return writeJsonFile(directory / fileName(), *json);
}

bool Project::save() const
{
    Json json;
    json[kVersionKey] = kFormatVersion;
    json[kNameKey] = m_name;
    json[kMainSceneKey] = idToJson(m_mainScene);
    json[kEngineVersionKey] = engineVersion();

    return writeJsonFile(projectFile(), json);
}

std::optional<std::filesystem::path> Project::mainScenePath() const
{
    return m_resources.path(m_mainScene);
}

} // namespace yqlora
