#pragma once

#include "yqlora/editor/ProjectList.hpp"
#include "yqlora/project/Project.hpp"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace yqlora {

class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

// Экран проектов без интерфейса: строки списка и действия над проектами.
// Ошибки - ключи перевода, их можно сразу показать через tr().
class ProjectManager {
public:
    struct Row {
        std::filesystem::path directory;
        std::string name;
        // Версия движка, который сохранил проект. Пусто, если неизвестно.
        std::string engineVersion;
        FormatCompatibility compatibility = FormatCompatibility::Unsupported;
        // Папки проекта нет на месте.
        bool missing = false;
    };

    // Чем кончилась попытка открыть проект.
    enum class OpenStatus {
        Opened,
        // Формат старый: сначала перевести (upgrade).
        NeedsUpgrade,
        // Формат новее движка или неизвестен.
        CannotOpen,
        // Папку убрали, строка помечена как missing.
        Missing,
    };

    struct OpenResult {
        OpenStatus status = OpenStatus::Missing;
        std::optional<Project> project;
    };

    // listFile - файл списка проектов в папке пользователя.
    ProjectManager(std::filesystem::path listFile, const SceneSerializer& serializer);

    std::span<const Row> rows() const
    {
        return m_rows;
    }

    // Перечитывает имена и версии проектов из их папок.
    void refresh();

    OpenResult open(std::size_t row);

    // Создаёт проект в location/name и добавляет в список.
    std::optional<Project> create(const std::filesystem::path& location, const std::string& name, std::string& error);

    // Добавляет в список существующий проект.
    bool import(const std::filesystem::path& directory, std::string& error);

    // Убирает проект из списка, а с deleteFolder - и его папку с диска.
    bool remove(std::size_t row, bool deleteFolder, std::string& error);

    // Переводит проект на текущий формат и открывает его.
    std::optional<Project> upgrade(const std::filesystem::path& directory, std::string& error);

    // Проект открыли: он поднимается в начало списка.
    void markOpened(const std::filesystem::path& directory);

private:
    // Сохраняет список и перечитывает строки.
    void commit();

    ProjectList m_list;
    const SceneSerializer& m_serializer;
    std::vector<Row> m_rows;
};

} // namespace yqlora::editor
