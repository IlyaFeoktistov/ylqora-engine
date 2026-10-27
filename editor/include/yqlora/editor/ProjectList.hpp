#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace yqlora::editor {

// Проекты, которые знает менеджер проектов, недавно открытые первыми.
// Хранится в файле в папке настроек пользователя. Сами проекты не трогает:
// удаление из списка не удаляет папку.
class ProjectList {
public:
    struct Entry {
        std::filesystem::path directory;
        // Секунды с 1970 года, 0 - ещё не открывали.
        std::int64_t lastOpened = 0;
    };

    explicit ProjectList(std::filesystem::path file);

    // Отсутствующий или испорченный файл - пустой список.
    void load();
    bool save() const;

    std::span<const Entry> entries() const
    {
        return m_entries;
    }

    // Добавляет проект, если его ещё нет в списке.
    void add(const std::filesystem::path& directory);
    void remove(const std::filesystem::path& directory);
    // Проект открыли: он поднимается в начало списка.
    void markOpened(const std::filesystem::path& directory);

private:
    std::vector<Entry>::iterator find(const std::filesystem::path& directory);
    void sort();

    std::filesystem::path m_file;
    std::vector<Entry> m_entries;
};

} // namespace yqlora::editor
