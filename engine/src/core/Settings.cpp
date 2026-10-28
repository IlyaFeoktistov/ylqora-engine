#include "yqlora/core/Settings.hpp"

#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"

namespace yqlora {

namespace {

constexpr const char* kLanguageKey = "language";

nlohmann::json readJson(const std::filesystem::path& file)
{
    const auto json = readJsonFile(file);
    if (!json || !json->is_object()) {
        if (std::filesystem::exists(file)) {
            log::warn("Settings file '{}' is corrupted, using defaults", toUtf8(file));
        }

        return nlohmann::json::object();
    }

    return *json;
}

} // namespace

Settings::Settings(std::filesystem::path file)
    : m_path(std::move(file))
{
}

bool Settings::load()
{
    if (!std::filesystem::exists(m_path)) {
        return false;
    }
    const auto json = readJson(m_path);
    if (json.empty()) {
        return false;
    }
    if (const auto it = json.find(kLanguageKey); it != json.end() && it->is_string()) {
        language = it->get<std::string>();
    }

    return true;
}

bool Settings::save() const
{
    // Читаем файл заново: в нём могут быть ключи, о которых этот код не знает.
    auto json = readJson(m_path);
    json[kLanguageKey] = language;
    if (!writeJsonFile(m_path, json)) {
        log::error("Cannot save settings to '{}'", toUtf8(m_path));

        return false;
    }

    return true;
}

} // namespace yqlora
