#include "yqlora/resource/ResourceDatabase.hpp"

#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/resource/BuiltinResources.hpp"

#include "core/JsonId.hpp"

#include <random>
#include <vector>

namespace yqlora {

namespace {

namespace fs = std::filesystem;
using Json = nlohmann::json;

constexpr const char* kIdKey = "id";

std::string toUtf8(const fs::path& path)
{
    const std::u8string text = path.generic_u8string();

    return {text.begin(), text.end()};
}

fs::path metaPath(const fs::path& file)
{
    fs::path result = file;
    result += ResourceDatabase::metaExtension();

    return result;
}

bool isMeta(const fs::path& file)
{
    return file.extension() == ResourceDatabase::metaExtension();
}

ResourceId readMeta(const fs::path& file)
{
    if (!fs::exists(metaPath(file))) {
        return 0;
    }
    const auto json = readJsonFile(metaPath(file));
    const auto id = json && json->is_object() && json->contains(kIdKey) ? idFromJson((*json)[kIdKey]) : std::nullopt;
    if (!id) {
        log::warn("Resource meta of '{}' is broken, the file gets a new id", toUtf8(file));

        return 0;
    }

    return *id;
}

bool writeMeta(const fs::path& file, ResourceId id)
{
    Json json;
    json[kIdKey] = idToJson(id);

    return writeJsonFile(metaPath(file), json);
}

// path лежит внутри folder (или равен ему).
bool isInside(const fs::path& path, const fs::path& folder)
{
    const auto [folderEnd, pathIt] = std::ranges::mismatch(folder, path);

    return folderEnd == folder.end();
}

} // namespace

void ResourceDatabase::scan(const fs::path& root, std::string_view skipName)
{
    m_root = root;
    m_paths.clear();
    m_ids.clear();

    std::error_code ec;
    auto it = fs::recursive_directory_iterator(root, ec);
    for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        const fs::path& absolute = it->path();
        const std::string name = toUtf8(absolute.filename());
        if (name.empty() || name.front() == '.') {
            if (it->is_directory(ec)) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file(ec) || isMeta(absolute) || (it.depth() == 0 && name == skipName)) {
            continue;
        }

        const fs::path relative = fs::relative(absolute, root, ec);
        ResourceId id = readMeta(absolute);
        if (id == 0 || builtin::isBuiltin(id) || m_paths.contains(id)) {
            id = newId();
            writeMeta(absolute, id);
        }
        remember(id, relative);
    }
}

ResourceId ResourceDatabase::add(const fs::path& path)
{
    if (const ResourceId known = id(path)) {
        return known;
    }
    const ResourceId created = newId();
    if (!writeMeta(m_root / path, created)) {
        log::error("Cannot write resource meta for '{}'", toUtf8(path));

        return 0;
    }
    remember(created, path);

    return created;
}

void ResourceDatabase::moved(const fs::path& from, const fs::path& to)
{
    std::vector<std::pair<ResourceId, fs::path>> changed;
    for (const auto& [id, path] : m_paths) {
        if (!isInside(path, from)) {
            continue;
        }
        fs::path target = to;
        for (auto part = std::next(path.begin(), std::distance(from.begin(), from.end())); part != path.end(); ++part) {
            target /= *part;
        }
        changed.emplace_back(id, target);
    }

    // У отдельного файла .meta лежит рядом и переезжает сама. В переименованной
    // папке .meta уже на месте вместе с файлами, их трогать не нужно.
    std::error_code ec;
    if (fs::exists(metaPath(m_root / from), ec)) {
        fs::rename(metaPath(m_root / from), metaPath(m_root / to), ec);
        if (ec) {
            log::error("Cannot move resource meta of '{}': {}", toUtf8(from), ec.message());
        }
    }
    for (const auto& [id, target] : changed) {
        forget(id);
        remember(id, target);
    }
}

void ResourceDatabase::removed(const fs::path& path)
{
    std::vector<ResourceId> gone;
    for (const auto& [id, known] : m_paths) {
        if (isInside(known, path)) {
            gone.push_back(id);
        }
    }
    std::error_code ec;
    fs::remove(metaPath(m_root / path), ec);
    for (const ResourceId id : gone) {
        forget(id);
    }
}

std::optional<fs::path> ResourceDatabase::path(ResourceId id) const
{
    const auto it = m_paths.find(id);

    return it != m_paths.end() ? std::optional(it->second) : std::nullopt;
}

ResourceId ResourceDatabase::id(const fs::path& path) const
{
    const auto it = m_ids.find(path.lexically_normal());

    return it != m_ids.end() ? it->second : 0;
}

void ResourceDatabase::remember(ResourceId id, const fs::path& path)
{
    const fs::path normal = path.lexically_normal();
    m_paths[id] = normal;
    m_ids[normal] = id;
}

void ResourceDatabase::forget(ResourceId id)
{
    if (const auto it = m_paths.find(id); it != m_paths.end()) {
        m_ids.erase(it->second);
        m_paths.erase(it);
    }
}

ResourceId ResourceDatabase::newId() const
{
    // 64 случайных бита: совпадение ID в разных проектах практически исключено.
    thread_local std::mt19937_64 engine{std::random_device{}()};
    ResourceId id = 0;
    while (id < builtin::kFirstProjectId || m_paths.contains(id)) {
        id = engine();
    }

    return id;
}

} // namespace yqlora
