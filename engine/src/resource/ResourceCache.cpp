#include "yqlora/resource/ResourceCache.hpp"

#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/project/Project.hpp"
#include "yqlora/render/BuiltinMeshes.hpp"
#include "yqlora/resource/BuiltinResources.hpp"
#include "yqlora/resource/ResourceLoader.hpp"

#include <algorithm>
#include <cctype>

namespace yqlora {

namespace fs = std::filesystem;

namespace {

std::string lowerExtension(const fs::path& file)
{
    std::string extension = toUtf8(file.extension());
    std::ranges::transform(extension, extension.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });

    return extension;
}

} // namespace

ResourceCache::ResourceCache()
{
    addBuiltins();
}

ResourceCache::~ResourceCache() = default;

void ResourceCache::addLoader(std::unique_ptr<ResourceLoader> loader)
{
    m_loaders.push_back(std::move(loader));
}

void ResourceCache::setProject(Project* project)
{
    m_project = project;
    m_files.clear();
    std::erase_if(m_entries, [](const auto& entry) {
        return !builtin::isBuiltin(entry.first);
    });
}

std::string_view ResourceCache::kindOf(const fs::path& file) const
{
    const ResourceLoader* loader = loaderFor(file);

    return loader ? loader->kind() : std::string_view();
}

const ResourceStatus* ResourceCache::status(ResourceId id) const
{
    const auto it = m_entries.find(id);

    return it != m_entries.end() ? &it->second.status : nullptr;
}

bool ResourceCache::reload(ResourceId id)
{
    const auto path = m_project ? m_project->resources().path(id) : std::nullopt;
    if (!path) {
        return false;
    }
    loadFile(*path);

    return true;
}

std::size_t ResourceCache::reloadChanged()
{
    std::size_t reloaded = 0;
    std::vector<fs::path> forgotten;
    for (const auto& [file, entry] : m_files) {
        // Файл переименовали или удалили через редактор: его ресурсы найдутся
        // по новому пути при следующем обращении.
        if (!m_project || m_project->resources().id(file) == 0) {
            forgotten.push_back(file);
            continue;
        }
        std::error_code ec;
        const fs::path full = m_project->directory() / file;
        const bool exists = fs::is_regular_file(full, ec);
        if (exists != entry.existed || (exists && fs::last_write_time(full, ec) != entry.time)) {
            forgotten.push_back(file);
            ++reloaded;
        }
    }
    for (const fs::path& file : forgotten) {
        const bool known = m_project && m_project->resources().id(file) != 0;
        if (known) {
            loadFile(file);
            continue;
        }
        for (const ResourceId id : m_files[file].ids) {
            m_entries.erase(id);
        }
        m_files.erase(file);
    }
    // Ресурсы, которых не было в проекте: их могли добавить.
    std::erase_if(m_entries, [this](const auto& entry) {
        const bool inFile = std::ranges::any_of(m_files, [&](const auto& file) {
            return std::ranges::find(file.second.ids, entry.first) != file.second.ids.end();
        });

        return !inFile && !builtin::isBuiltin(entry.first) && entry.second.status.state == ResourceState::Missing;
    });

    return reloaded;
}

ResourceCache::Found ResourceCache::getData(ResourceId id, std::string_view kind)
{
    if (id == 0) {
        return {nullptr, 0};
    }
    auto it = m_entries.find(id);
    if (it == m_entries.end()) {
        if (const auto path = m_project ? m_project->resources().path(id) : std::nullopt) {
            loadFile(*path);
        }
        it = m_entries.find(id);
    }
    if (it == m_entries.end()) {
        log::warn("Resource {:016x} is not in the project", id);
        setEntry(id, {{}, nullptr, {ResourceState::Missing, 0, "The resource is not in the project"}});
        it = m_entries.find(id);
    }
    const Entry& entry = it->second;
    const bool fits = entry.data && entry.kind == kind;

    return {fits ? entry.data.get() : nullptr, entry.status.revision};
}

void ResourceCache::addBuiltins()
{
    for (const BuiltinMesh& mesh : builtinMeshes()) {
        setEntry(
            mesh.id,
            {MeshData::kResourceKind, std::make_shared<const MeshData>(mesh.make()), {ResourceState::Ready, 0, {}}}
        );
    }
}

void ResourceCache::loadFile(const fs::path& file)
{
    ResourceDatabase& database = m_project->resources();
    const fs::path full = m_project->directory() / file;
    const ResourceId own = database.id(file);
    FileEntry loaded;
    std::error_code ec;
    loaded.existed = fs::is_regular_file(full, ec);
    if (loaded.existed) {
        loaded.time = fs::last_write_time(full, ec);
    }

    // Новые записи собираются целиком, прежние заменяются только потом.
    std::map<ResourceId, Entry> fresh;
    const auto fail = [&](ResourceState state, std::string message) {
        fresh[own] = {{}, nullptr, {state, 0, std::move(message)}};
    };
    const ResourceLoader* loader = loaderFor(file);
    if (!loaded.existed) {
        fail(ResourceState::Missing, "The file is missing");
    } else if (!loader) {
        fail(ResourceState::Error, "No loader for files " + lowerExtension(file));
    } else if (LoadResult result = loader->load(request(own, full)); !result.error.empty()) {
        fail(ResourceState::Error, std::move(result.error));
    } else {
        std::vector<std::string> keys;
        for (const LoadedResource& resource : result.resources) {
            if (!resource.key.empty()) {
                keys.push_back(resource.key);
            }
        }
        const std::vector<ResourceId> nested = database.assignSubresources(own, keys);
        auto nextNested = nested.begin();
        for (LoadedResource& resource : result.resources) {
            const ResourceId id = resource.key.empty() ? own : *nextNested++;
            fresh[id] = {resource.kind, std::move(resource.data), {ResourceState::Ready, 0, {}}};
        }
        if (!fresh.contains(own)) {
            fail(ResourceState::Error, "The file holds no resource of its own");
        }
    }

    // Ресурсы, которые были в файле раньше, а теперь их нет.
    if (const auto previous = m_files.find(file); previous != m_files.end()) {
        for (const ResourceId id : previous->second.ids) {
            if (!fresh.contains(id)) {
                fresh[id] = {{}, nullptr, {ResourceState::Missing, 0, "The resource is not in the file any more"}};
            }
        }
    }
    for (auto& [id, entry] : fresh) {
        if (entry.status.state == ResourceState::Missing) {
            log::warn("Resource '{}': {}", toGenericUtf8(file), entry.status.message);
        } else if (entry.status.state == ResourceState::Error) {
            log::error("Resource '{}' is not loaded: {}", toGenericUtf8(file), entry.status.message);
        }
        loaded.ids.push_back(id);
        setEntry(id, std::move(entry));
    }
    m_files[file] = std::move(loaded);
}

LoadRequest ResourceCache::request(ResourceId file, const fs::path& full) const
{
    LoadRequest request{full};
    const auto import = nlohmann::json::parse(m_project->resources().importSettings(file), nullptr, false);
    if (import.is_object()) {
        request.import = import;
    }

    return request;
}

const ResourceLoader* ResourceCache::loaderFor(const fs::path& file) const
{
    const std::string extension = lowerExtension(file);
    for (const auto& loader : m_loaders) {
        if (std::ranges::find(loader->extensions(), extension) != loader->extensions().end()) {
            return loader.get();
        }
    }

    return nullptr;
}

void ResourceCache::setEntry(ResourceId id, Entry entry)
{
    entry.status.revision = ++m_lastRevision;
    m_entries[id] = std::move(entry);
}

} // namespace yqlora
