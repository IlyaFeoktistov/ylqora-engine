#include "yqlora/project/ProjectSettings.hpp"

#include "yqlora/core/FileFormat.hpp"
#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/reflection/PropertyJson.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace yqlora {

namespace {

// Формат файла раздела. Растёт вместе с форматом проекта (см. Project::formatVersion).
constexpr int kFirstFormatVersion = 1;
constexpr std::array<Migration, 0> kMigrations{};
constexpr FileFormat kFormat{"Project settings", kFirstFormatVersion, kMigrations};

// Значения свойств лежат отдельно от версии формата: имя свойства не
// столкнётся с ключом версии.
constexpr const char* kValuesKey = "values";

} // namespace

void ProjectSettings::load(const TypeRegistry& registry, const std::filesystem::path& projectDirectory)
{
    m_registry = &registry;
    m_directory = projectDirectory / directoryName();
    m_sections.clear();

    for (const auto& type : registry.types()) {
        if (type->kind() != TypeKind::Settings) {
            continue;
        }
        auto section = type->createSettings();
        if (!section) {
            continue;
        }
        const std::filesystem::path path = file(*section);
        if (std::filesystem::exists(path)) {
            const auto json = readJsonFile(path);
            if (!json || !json->is_object()
                || kFormat.compatibility(FileFormat::versionOf(*json)) != FormatCompatibility::Current) {
                log::warn("Project settings '{}' are not read, using defaults", toUtf8(path));
            } else if (const auto values = json->find(kValuesKey); values != json->end()) {
                propertiesFromJson(registry, *type, *section, *values);
            }
        }
        m_sections.push_back(std::move(section));
    }
}

bool ProjectSettings::save(const SettingsSection& section)
{
    ++m_revision;
    const TypeInfo* type = m_registry ? m_registry->find(section.typeName()) : nullptr;
    if (!type) {
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(m_directory, ec);
    nlohmann::json json;
    kFormat.stamp(json);
    json[kValuesKey] = propertiesToJson(*m_registry, *type, section);

    return writeJsonFile(file(section), json);
}

bool ProjectSettings::upgradeFiles(const std::filesystem::path& projectDirectory)
{
    std::vector<std::pair<std::filesystem::path, nlohmann::json>> files;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(projectDirectory / directoryName(), ec)) {
        if (!entry.is_regular_file(ec) || entry.path().extension() != fileExtension()) {
            continue;
        }
        auto json = readJsonFile(entry.path());
        if (!json || !json->is_object() || !kFormat.migrate(*json)) {
            log::error("Project upgrade: settings '{}' are not read, nothing is changed", toUtf8(entry.path()));

            return false;
        }
        files.emplace_back(entry.path(), std::move(*json));
    }
    for (const auto& [path, json] : files) {
        if (!writeJsonFile(path, json)) {
            log::error("Project upgrade: cannot write settings '{}'", toUtf8(path));

            return false;
        }
    }

    return true;
}

SettingsSection* ProjectSettings::find(std::string_view typeName)
{
    const auto it = std::ranges::find(m_sections, typeName, &SettingsSection::typeName);

    return it != m_sections.end() ? it->get() : nullptr;
}

const SettingsSection* ProjectSettings::find(std::string_view typeName) const
{
    const auto it = std::ranges::find(m_sections, typeName, &SettingsSection::typeName);

    return it != m_sections.end() ? it->get() : nullptr;
}

std::filesystem::path ProjectSettings::file(const SettingsSection& section) const
{
    const TypeInfo* type = m_registry->find(section.typeName());

    return m_directory / fromUtf8(type->settingsFile() + std::string(fileExtension()));
}

} // namespace yqlora
