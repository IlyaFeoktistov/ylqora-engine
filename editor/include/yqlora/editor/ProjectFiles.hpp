#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

class Project;
class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

// Файлы внутри папки проекта: дерево для панели файлов и операции над ним.
// Все пути - относительно папки проекта, выйти за её пределы нельзя.
// Вместе с файлами обновляются ресурсы проекта, файлы .meta переезжают следом.
class ProjectFiles {
public:
    struct Entry {
        std::string name;
        std::filesystem::path path;
        bool directory = false;
        std::vector<Entry> children;
    };

    explicit ProjectFiles(Project& project);

    static bool isScene(const std::filesystem::path& path);

    // Дерево файлов: папки первыми, внутри по алфавиту. Файл проекта, файлы
    // .meta и скрытые файлы (имя с точки) не показываются.
    Entry scan() const;

    // Новая пустая сцена в папке folder: "New Scene.yscene", "New Scene 2.yscene"...
    std::optional<std::filesystem::path> createScene(
        const std::filesystem::path& folder,
        const SceneSerializer& serializer
    );

    std::optional<std::filesystem::path> createFolder(const std::filesystem::path& folder);

    // Переименовывает файл или папку в той же папке. Возвращает новый путь;
    // nullopt, если имя недопустимо, занято или путь трогать нельзя.
    std::optional<std::filesystem::path> rename(const std::filesystem::path& path, const std::string& newName);

    // Удаляет файл или папку со всем содержимым.
    bool remove(const std::filesystem::path& path);

    // Путь указывает внутрь проекта и это не сам файл проекта и не корень папки.
    bool canModify(const std::filesystem::path& path) const;

private:
    Project& m_project;
};

} // namespace yqlora::editor
