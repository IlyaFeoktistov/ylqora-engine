#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

class Project;
class ResourceLoader;
struct LoadRequest;

enum class ResourceState {
    Ready,
    // Нет файла или такого ресурса в проекте (или в файле). Ссылки остаются
    // и снова работают, когда ресурс появится.
    Missing,
    // Файл есть, но не прочитался: прежние данные под видом текущих не отдаются.
    Error,
};

struct ResourceStatus {
    ResourceState state = ResourceState::Missing;
    // Растёт при каждой загрузке и перезагрузке и никогда не повторяется,
    // даже для другого ID: по паре ID и ревизии бэкенд узнаёт свою копию.
    std::uint64_t revision = 0;
    // Причина для Missing и Error.
    std::string message;
};

// Данные ресурса и их ревизия. data - nullptr, если ресурса нет, он не
// загрузился или он другого вида.
template <typename T>
struct LoadedData {
    const T* data = nullptr;
    std::uint64_t revision = 0;

    explicit operator bool() const
    {
        return data != nullptr;
    }
};

// Загруженные ресурсы по ID: встроенные (всегда) и файлы открытого проекта.
// Файл читается при первом обращении к любому его ресурсу, загрузчиком по
// расширению. Данные живут до перезагрузки их файла или смены проекта:
// пользователи берут их заново каждый кадр и между кадрами указатели не
// хранят. Только из главного потока.
class YQLORA_API ResourceCache {
public:
    ResourceCache();
    ~ResourceCache();

    ResourceCache(const ResourceCache&) = delete;
    ResourceCache& operator=(const ResourceCache&) = delete;

    void addLoader(std::unique_ptr<ResourceLoader> loader);

    // Проект, чьи файлы читаются. Ресурсы прежнего проекта забываются,
    // встроенные остаются. nullptr - проекта нет.
    // Файлы вложенных ресурсов получают их ID в своих .meta при загрузке,
    // поэтому проект изменяемый.
    void setProject(Project* project);

    // T - тип данных с T::kResourceKind (MeshData).
    template <typename T>
    LoadedData<T> get(ResourceId id)
    {
        const auto [data, revision] = getData(id, T::kResourceKind);

        return {static_cast<const T*>(data), revision};
    }

    template <typename T>
    const T* find(ResourceId id)
    {
        return get<T>(id).data;
    }

    // nullptr, если к ресурсу ещё не обращались.
    const ResourceStatus* status(ResourceId id) const;

    // Перечитывает файл ресурса id (изменился на диске, поменяли настройки
    // импорта). Новые данные заменяют старые только после загрузки. false,
    // если файла у ресурса нет.
    bool reload(ResourceId id);

    // Перечитывает загруженные файлы, которые изменились, пропали или
    // появились с последней загрузки; ресурсы, которых не было, ищутся
    // заново при следующем обращении. Возвращает число перечитанных файлов.
    std::size_t reloadChanged();

private:
    struct Entry {
        std::string_view kind;
        std::shared_ptr<const void> data;
        ResourceStatus status;
    };

    // Загруженный файл и то, каким он был при загрузке.
    struct FileEntry {
        bool existed = false;
        std::filesystem::file_time_type time;
        std::vector<ResourceId> ids;
    };

    struct Found {
        const void* data;
        std::uint64_t revision;
    };

    Found getData(ResourceId id, std::string_view kind);
    void addBuiltins();
    // Читает файл (путь от папки проекта) и заменяет записи его ресурсов.
    void loadFile(const std::filesystem::path& file);
    const ResourceLoader* loaderFor(const std::filesystem::path& file) const;
    LoadRequest request(ResourceId file, const std::filesystem::path& full) const;
    void setEntry(ResourceId id, Entry entry);

    std::vector<std::unique_ptr<ResourceLoader>> m_loaders;
    Project* m_project = nullptr;
    std::map<ResourceId, Entry> m_entries;
    std::map<std::filesystem::path, FileEntry> m_files;
    std::uint64_t m_lastRevision = 0;
};

} // namespace yqlora
