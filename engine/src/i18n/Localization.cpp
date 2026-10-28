#include "yqlora/i18n/Localization.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>

namespace yqlora {

namespace {

const Localization* g_active = nullptr;

void flatten(
    const nlohmann::json& node,
    const std::string& prefix,
    std::map<std::string, std::string, std::less<>>& out
)
{
    for (const auto& [key, value] : node.items()) {
        const std::string fullKey = prefix.empty() ? key : prefix + "." + key;
        if (value.is_object()) {
            flatten(value, fullKey, out);
        } else if (value.is_string()) {
            out[fullKey] = value.get<std::string>();
        }
    }
}

std::string normalizeLanguageCode(std::string_view code)
{
    std::string result;
    for (const char c : code) {
        if (c == '_' || c == '-' || c == '.') {
            break;
        }
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return result;
}

} // namespace

Localization::Localization(std::filesystem::path localesDir)
    : m_dir(std::move(localesDir))
{
}

bool Localization::loadFile(std::string_view language, Strings& out) const
{
    const auto file = m_dir / (std::string(language) + ".json");
    std::ifstream in(file);
    if (!in) {
        log::error("Locale file '{}' not found", toUtf8(file));

        return false;
    }
    const auto json = nlohmann::json::parse(in, nullptr, /*allow_exceptions=*/false);
    if (json.is_discarded() || !json.is_object()) {
        log::error("Locale file '{}' is not a valid JSON object", toUtf8(file));

        return false;
    }
    out.clear();
    flatten(json, {}, out);

    return true;
}

std::string Localization::displayName(std::string_view language) const
{
    Strings strings;
    if (!loadFile(language, strings)) {
        return std::string(language);
    }
    const auto it = strings.find("language.name");

    return it != strings.end() ? it->second : std::string(language);
}

bool Localization::load(std::string_view language)
{
    Strings strings;
    if (!loadFile(language, strings)) {
        return false;
    }
    if (language != kFallbackLanguage) {
        loadFile(kFallbackLanguage, m_fallback);
    } else {
        m_fallback.clear();
    }
    m_strings = std::move(strings);
    m_language = language;
    log::info("Language: {}", m_language);

    return true;
}

std::string Localization::translate(std::string_view key) const
{
    if (const auto it = m_strings.find(key); it != m_strings.end()) {
        return it->second;
    }
    if (const auto it = m_fallback.find(key); it != m_fallback.end()) {
        return it->second;
    }

    return std::string(key);
}

std::vector<std::string> Localization::availableLanguages() const
{
    std::vector<std::string> result;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(m_dir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            result.push_back(toUtf8(entry.path().stem()));
        }
    }
    std::ranges::sort(result);

    return result;
}

std::string Localization::pickLanguage(std::span<const std::string> preferred, std::span<const std::string> available)
{
    for (const auto& code : preferred) {
        const auto normalized = normalizeLanguageCode(code);
        if (std::ranges::find(available, normalized) != available.end()) {
            return normalized;
        }
    }

    return std::string(kFallbackLanguage);
}

void Localization::setActive(const Localization* localization)
{
    g_active = localization;
}

std::string tr(std::string_view key)
{
    return g_active ? g_active->translate(key) : std::string(key);
}

} // namespace yqlora
