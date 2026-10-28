#include "yqlora/project/Project.hpp"

#include "yqlora/core/FileFormat.hpp"
#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/JsonId.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/core/Version.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <array>
#include <memory>
#include <utility>
#include <vector>

namespace yqlora {

namespace {

using Json = nlohmann::json;

// Переводы файла проекта со старых версий. Сцены переводит их собственный
// формат в SceneSerializer. Пока формат один, переводов нет.
constexpr int kFirstFormatVersion = 1;
constexpr std::array<Migration, 0> kMigrations{};
constexpr FileFormat kFormat{"Project", kFirstFormatVersion, kMigrations};

// Ключи файла проекта.
constexpr const char* kNameKey = "name";
constexpr const char* kMainSceneKey = "mainScene";
constexpr const char* kEngineVersionKey = "engine";

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
    return kFormat.current();
}

std::optional<Project> Project::open(const std::filesystem::path& directory)
{
    const auto json = readFile(directory);
    if (!json) {
        return std::nullopt;
    }
    if (kFormat.compatibility(FileFormat::versionOf(*json)) != FormatCompatibility::Current) {
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
    info.formatVersion = FileFormat::versionOf(*json);
    info.engineVersion = json->value(kEngineVersionKey, "");
    info.compatibility = kFormat.compatibility(info.formatVersion);

    return info;
}

bool Project::upgrade(const std::filesystem::path& directory, const SceneSerializer& serializer)
{
    auto json = readFile(directory);
    if (!json) {
        return false;
    }
    const FormatCompatibility compatibility = kFormat.compatibility(FileFormat::versionOf(*json));
    if (compatibility != FormatCompatibility::Older && compatibility != FormatCompatibility::Current) {
        return false;
    }

    // Чтение переводит сцену на текущий формат, запись сохраняет уже в нём.
    // Сцену нельзя переместить в памяти (на неё указывают узлы), поэтому по указателю.
    std::vector<std::pair<std::filesystem::path, std::unique_ptr<Scene>>> scenes;
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(directory, ec);
         !ec && it != std::filesystem::recursive_directory_iterator();
         it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == SceneSerializer::fileExtension()) {
            Scene& scene = *scenes.emplace_back(it->path(), std::make_unique<Scene>()).second;
            if (!serializer.load(scene, it->path())) {
                log::error("Project upgrade: scene '{}' is not read, nothing is changed", toUtf8(it->path()));

                return false;
            }
        }
    }
    for (const auto& [path, scene] : scenes) {
        if (!serializer.save(*scene, path)) {
            log::error("Project upgrade: cannot write scene '{}'", toUtf8(path));

            return false;
        }
    }

    if (!kFormat.migrate(*json)) {
        return false;
    }
    (*json)[kEngineVersionKey] = engineVersion();

    return writeJsonFile(directory / fileName(), *json);
}

bool Project::save() const
{
    Json json;
    kFormat.stamp(json);
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
