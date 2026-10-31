#pragma once

#include <filesystem>
#include <optional>

namespace yqlora::editor {

// Копии библиотеки игры, которые загружает редактор: собранную библиотеку
// Ninja перезаписывает при следующей сборке, а загруженный файл Windows не
// даёт ни перезаписать, ни удалить. Каждая загрузка - новая копия
// <проект>/.build/reload/game_<n>, поэтому каждая перезагрузка видна на диске.
class ModuleCopies {
public:
    explicit ModuleCopies(const std::filesystem::path& projectDirectory);

    // Копирует library под новым именем. nullopt, если не вышло (причина в логе).
    std::optional<std::filesystem::path> copy(const std::filesystem::path& library);

    // Удаляет все копии, кроме inUse (загруженной). Заблокированные остаются
    // до следующего раза.
    void removeUnused(const std::filesystem::path& inUse = {}) const;

    const std::filesystem::path& directory() const
    {
        return m_directory;
    }

private:
    std::filesystem::path m_directory;
    int m_next = 1;
};

} // namespace yqlora::editor
