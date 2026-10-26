#include "yqlora/core/Settings.hpp"

#include "yqlora/core/Log.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <system_error>

namespace yqlora {

namespace {

nlohmann::json readJson(const std::filesystem::path& file)
{
    std::ifstream in(file);
    if (!in) {
        return nlohmann::json::object();
    }
    auto json = nlohmann::json::parse(in, nullptr, /*allow_exceptions=*/false);
    if (json.is_discarded() || !json.is_object()) {
        log::warn("Settings file '{}' is corrupted, using defaults", file.string());

        return nlohmann::json::object();
    }

    return json;
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
    if (const auto it = json.find("language"); it != json.end() && it->is_string()) {
        language = it->get<std::string>();
    }

    return true;
}

bool Settings::save() const
{
    auto json = readJson(m_path);
    json["language"] = language;

    std::error_code ec;
    std::filesystem::create_directories(m_path.parent_path(), ec);

    // Пишем во временный файл и переименовываем: при сбое старые настройки не пропадут.
    auto tmp = m_path;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) {
            log::error("Cannot write settings to '{}'", tmp.string());

            return false;
        }
        out << json.dump(4) << '\n';
    }
    std::filesystem::rename(tmp, m_path, ec);
    if (ec) {
        log::error("Cannot save settings to '{}': {}", m_path.string(), ec.message());

        return false;
    }

    return true;
}

} // namespace yqlora
