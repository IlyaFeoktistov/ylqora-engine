#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <string_view>

namespace yqlora {

// ID и пути ресурсов проекта. ID хранится в файле <ресурс>.meta рядом с самим
// ресурсом, поэтому переживает переименование и перенос, если .meta переезжает
// вместе с файлом (так делает редактор). Все пути - относительно папки проекта.
class YQLORA_API ResourceDatabase {
public:
    static constexpr std::string_view metaExtension()
    {
        return ".meta";
    }

    // Обходит папку проекта. Файлу без .meta создаёт .meta с новым ID. Если
    // ID повторяется (файл скопировали вместе с .meta), копия получает новый.
    // Скрытые файлы и пропущенные имена в корне (файл проекта, папка
    // настроек) не ресурсы.
    void scan(const std::filesystem::path& root, std::span<const std::string_view> skipNames = {});

    // Новый файл в проекте: .meta с новым ID. Если файл уже известен, его ID.
    ResourceId add(const std::filesystem::path& path);

    // Файл или папку уже переименовали: .meta переезжают следом, ID остаются.
    void moved(const std::filesystem::path& from, const std::filesystem::path& to);

    // Файл или папку уже удалили: удаляет их .meta и забывает ID.
    void removed(const std::filesystem::path& path);

    // nullopt, если такого ресурса в проекте нет.
    std::optional<std::filesystem::path> path(ResourceId id) const;

    // 0, если файл не ресурс проекта.
    ResourceId id(const std::filesystem::path& path) const;

    std::size_t size() const
    {
        return m_paths.size();
    }

    // Все ресурсы проекта: ID и путь от папки проекта, по порядку ID.
    const std::map<ResourceId, std::filesystem::path>& paths() const
    {
        return m_paths;
    }

private:
    void remember(ResourceId id, const std::filesystem::path& path);
    void forget(ResourceId id);
    ResourceId newId() const;

    std::filesystem::path m_root;
    std::map<ResourceId, std::filesystem::path> m_paths;
    std::map<std::filesystem::path, ResourceId> m_ids;
};

} // namespace yqlora
