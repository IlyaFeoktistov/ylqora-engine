#include "yqlora/resource/ResourceDatabase.hpp"

#include "yqlora/core/FileFormat.hpp"
#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/JsonId.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/resource/BuiltinResources.hpp"

#include <algorithm>
#include <array>
#include <random>
#include <vector>

namespace yqlora {

namespace {

namespace fs = std::filesystem;
using Json = nlohmann::json;

constexpr const char* kIdKey = "id";
constexpr const char* kImportKey = "import";
constexpr const char* kSubresourcesKey = "subresources";
constexpr const char* kNoImportSettings = "{}";

constexpr int kFirstMetaVersion = 1;
constexpr std::array<Migration, 0> kMetaMigrations{};
constexpr FileFormat kMetaFormat{"Resource meta", kFirstMetaVersion, kMetaMigrations};

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

struct MetaFile {
    ResourceId id = 0;
    std::map<std::string, ResourceId> subresources;
    std::string import = kNoImportSettings;
};

// nullopt, если .meta нет или она не читается: тогда файл получит новый ID.
std::optional<MetaFile> readMeta(const fs::path& file)
{
    if (!fs::exists(metaPath(file))) {
        return std::nullopt;
    }
    const auto json = readJsonFile(metaPath(file));
    const bool current = json && json->is_object()
                         && kMetaFormat.compatibility(FileFormat::versionOf(*json)) == FormatCompatibility::Current;
    const auto id = current && json->contains(kIdKey) ? idFromJson((*json)[kIdKey]) : std::nullopt;
    if (!id) {
        log::warn("Resource meta of '{}' is broken, the file gets a new id", toUtf8(file));

        return std::nullopt;
    }
    MetaFile meta{*id, {}, kNoImportSettings};
    if (const auto import = json->find(kImportKey); import != json->end() && import->is_object()) {
        meta.import = import->dump();
    }
    if (const auto nested = json->find(kSubresourcesKey); nested != json->end() && nested->is_object()) {
        for (const auto& [key, value] : nested->items()) {
            // Битый ID вложенного ресурса - 0: при обходе он получит новый.
            meta.subresources[key] = idFromJson(value).value_or(0);
        }
    }

    return meta;
}

// path лежит внутри folder (или равен ему).
bool isInside(const fs::path& path, const fs::path& folder)
{
    const auto [folderEnd, pathIt] = std::ranges::mismatch(folder, path);

    return folderEnd == folder.end();
}

} // namespace

void ResourceDatabase::scan(
    const fs::path& root,
    std::span<const std::string_view> skipNames,
    std::span<const std::string_view> skipExtensions
)
{
    m_root = root;
    m_paths.clear();
    m_ids.clear();
    m_meta.clear();
    m_nested.clear();

    std::error_code ec;
    auto it = fs::recursive_directory_iterator(root, ec);
    for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        const fs::path& absolute = it->path();
        const std::string name = toUtf8(absolute.filename());
        const bool skipped = it.depth() == 0 && std::ranges::find(skipNames, name) != skipNames.end();
        if (name.empty() || name.front() == '.' || skipped) {
            if (it->is_directory(ec)) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file(ec) || isMeta(absolute)
            || std::ranges::find(skipExtensions, toUtf8(absolute.extension())) != skipExtensions.end()) {
            continue;
        }

        MetaFile meta = readMeta(absolute).value_or(MetaFile{});
        bool rewrite = false;
        // Копия файла вместе с .meta: и её вложенные ID уже заняты оригиналом,
        // поэтому ниже они тоже станут новыми.
        if (meta.id == 0 || builtin::isBuiltin(meta.id) || known(meta.id)) {
            meta.id = newId();
            rewrite = true;
        }
        remember(meta.id, fs::relative(absolute, root, ec), {{}, std::move(meta.import)});
        for (auto& [key, nested] : meta.subresources) {
            if (nested == 0 || builtin::isBuiltin(nested) || known(nested)) {
                nested = newId();
                rewrite = true;
            }
            m_nested[nested] = {meta.id, key};
            m_meta[meta.id].subresources[key] = nested;
        }
        if (rewrite) {
            writeMeta(meta.id);
        }
    }
}

ResourceId ResourceDatabase::add(const fs::path& path)
{
    if (const ResourceId existing = id(path)) {
        return existing;
    }
    const ResourceId created = newId();
    remember(created, path, {});
    if (!writeMeta(created)) {
        log::error("Cannot write resource meta for '{}'", toUtf8(path));
        forget(created);

        return 0;
    }

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
    // Вложенные ресурсы знают свой файл по ID, им переезд не нужен.
    for (const auto& [id, target] : changed) {
        m_ids.erase(m_paths[id]);
        const fs::path normal = target.lexically_normal();
        m_paths[id] = normal;
        m_ids[normal] = id;
    }
}

void ResourceDatabase::removed(const fs::path& path)
{
    std::vector<ResourceId> gone;
    for (const auto& [id, file] : m_paths) {
        if (isInside(file, path)) {
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
    const auto it = m_paths.find(fileOf(id));

    return it != m_paths.end() ? std::optional(it->second) : std::nullopt;
}

ResourceId ResourceDatabase::id(const fs::path& path) const
{
    const auto it = m_ids.find(path.lexically_normal());

    return it != m_ids.end() ? it->second : 0;
}

ResourceId ResourceDatabase::fileOf(ResourceId id) const
{
    if (const auto nested = m_nested.find(id); nested != m_nested.end()) {
        return nested->second.file;
    }

    return m_paths.contains(id) ? id : 0;
}

std::string_view ResourceDatabase::key(ResourceId id) const
{
    const auto nested = m_nested.find(id);

    return nested != m_nested.end() ? std::string_view(nested->second.key) : std::string_view();
}

std::vector<ResourceId> ResourceDatabase::assignSubresources(ResourceId file, std::span<const std::string> keys)
{
    const auto meta = m_meta.find(file);
    if (meta == m_meta.end()) {
        return {};
    }
    std::vector<ResourceId> ids;
    bool added = false;
    for (const std::string& key : keys) {
        auto [it, inserted] = meta->second.subresources.try_emplace(key, 0);
        if (inserted) {
            it->second = newId();
            m_nested[it->second] = {file, key};
            added = true;
        }
        ids.push_back(it->second);
    }
    if (added && !writeMeta(file)) {
        log::error("Cannot write resource meta for '{}'", toUtf8(m_paths[file]));
    }

    return ids;
}

const std::map<std::string, ResourceId>& ResourceDatabase::subresources(ResourceId file) const
{
    static const std::map<std::string, ResourceId> kNone;
    const auto meta = m_meta.find(file);

    return meta != m_meta.end() ? meta->second.subresources : kNone;
}

const std::string& ResourceDatabase::importSettings(ResourceId file) const
{
    static const std::string kNone = kNoImportSettings;
    const auto meta = m_meta.find(file);

    return meta != m_meta.end() ? meta->second.import : kNone;
}

bool ResourceDatabase::setImportSettings(ResourceId file, std::string json)
{
    const auto meta = m_meta.find(file);
    const Json parsed = Json::parse(json, nullptr, /*allow_exceptions=*/false);
    if (meta == m_meta.end() || !parsed.is_object()) {
        return false;
    }
    meta->second.import = parsed.dump();

    return writeMeta(file);
}

void ResourceDatabase::remember(ResourceId id, const fs::path& path, Meta meta)
{
    const fs::path normal = path.lexically_normal();
    m_paths[id] = normal;
    m_ids[normal] = id;
    if (meta.import.empty()) {
        meta.import = kNoImportSettings;
    }
    for (const auto& [key, nested] : meta.subresources) {
        m_nested[nested] = {id, key};
    }
    m_meta[id] = std::move(meta);
}

void ResourceDatabase::forget(ResourceId id)
{
    if (const auto it = m_paths.find(id); it != m_paths.end()) {
        m_ids.erase(it->second);
        m_paths.erase(it);
    }
    if (const auto meta = m_meta.find(id); meta != m_meta.end()) {
        for (const auto& [key, nested] : meta->second.subresources) {
            m_nested.erase(nested);
        }
        m_meta.erase(meta);
    }
}

bool ResourceDatabase::writeMeta(ResourceId id) const
{
    const auto path = m_paths.find(id);
    const auto meta = m_meta.find(id);
    if (path == m_paths.end() || meta == m_meta.end()) {
        return false;
    }
    Json json;
    kMetaFormat.stamp(json);
    json[kIdKey] = idToJson(id);
    if (meta->second.import != kNoImportSettings) {
        json[kImportKey] = Json::parse(meta->second.import);
    }
    if (!meta->second.subresources.empty()) {
        Json nested = Json::object();
        for (const auto& [key, value] : meta->second.subresources) {
            nested[key] = idToJson(value);
        }
        json[kSubresourcesKey] = std::move(nested);
    }

    return writeJsonFile(metaPath(m_root / path->second), json);
}

bool ResourceDatabase::known(ResourceId id) const
{
    return m_paths.contains(id) || m_nested.contains(id);
}

ResourceId ResourceDatabase::newId() const
{
    // 64 случайных бита: совпадение ID в разных проектах практически исключено.
    thread_local std::mt19937_64 engine{std::random_device{}()};
    ResourceId id = 0;
    while (id < builtin::kFirstProjectId || known(id)) {
        id = engine();
    }

    return id;
}

} // namespace yqlora
