#include "yqlora/editor/ProjectList.hpp"

#include "yqlora/core/JsonFile.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>

namespace yqlora::editor {

namespace {

using Json = nlohmann::json;

constexpr const char* kPathKey = "path";
constexpr const char* kLastOpenedKey = "lastOpened";

std::int64_t now()
{
    const auto sinceEpoch = std::chrono::system_clock::now().time_since_epoch();

    return std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch).count();
}

// Один и тот же проект не должен попасть в список дважды из-за "a/b" и "a/b/".
std::filesystem::path normalized(const std::filesystem::path& directory)
{
    std::filesystem::path result = directory.lexically_normal();
    if (!result.has_filename() && result.has_parent_path()) {
        result = result.parent_path();
    }

    return result;
}

} // namespace

ProjectList::ProjectList(std::filesystem::path file)
    : m_file(std::move(file))
{
}

void ProjectList::load()
{
    m_entries.clear();
    const auto json = readJsonFile(m_file);
    if (!json || !json->is_array()) {
        return;
    }
    for (const Json& item : *json) {
        if (!item.is_object() || !item.contains(kPathKey) || !item[kPathKey].is_string()) {
            continue;
        }
        const std::string path = item[kPathKey].get<std::string>();
        Entry entry;
        entry.directory = normalized(std::filesystem::path(std::u8string(path.begin(), path.end())));
        if (const auto it = item.find(kLastOpenedKey); it != item.end() && it->is_number_integer()) {
            entry.lastOpened = it->get<std::int64_t>();
        }
        m_entries.push_back(std::move(entry));
    }
    sort();
}

bool ProjectList::save() const
{
    Json json = Json::array();
    for (const Entry& entry : m_entries) {
        const std::u8string path = entry.directory.generic_u8string();
        json.push_back({{kPathKey, std::string(path.begin(), path.end())}, {kLastOpenedKey, entry.lastOpened}});
    }

    return writeJsonFile(m_file, json);
}

void ProjectList::add(const std::filesystem::path& directory)
{
    if (find(directory) != m_entries.end()) {
        return;
    }
    m_entries.push_back({normalized(directory), 0});
    sort();
}

void ProjectList::remove(const std::filesystem::path& directory)
{
    const auto it = find(directory);
    if (it != m_entries.end()) {
        m_entries.erase(it);
    }
}

void ProjectList::markOpened(const std::filesystem::path& directory)
{
    auto it = find(directory);
    if (it == m_entries.end()) {
        m_entries.push_back({normalized(directory), 0});
        it = std::prev(m_entries.end());
    }
    // Строго позже всех остальных: два открытия за одну секунду тоже встанут по порядку.
    std::int64_t latest = now();
    for (const Entry& entry : m_entries) {
        if (entry.directory != it->directory) {
            latest = std::max(latest, entry.lastOpened + 1);
        }
    }
    it->lastOpened = latest;
    sort();
}

std::vector<ProjectList::Entry>::iterator ProjectList::find(const std::filesystem::path& directory)
{
    const std::filesystem::path key = normalized(directory);

    return std::ranges::find_if(m_entries, [&](const Entry& entry) {
        return entry.directory == key;
    });
}

void ProjectList::sort()
{
    std::ranges::stable_sort(m_entries, [](const Entry& a, const Entry& b) {
        return a.lastOpened > b.lastOpened;
    });
}

} // namespace yqlora::editor
