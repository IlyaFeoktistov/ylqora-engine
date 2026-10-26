#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace yqlora {

struct Project;
class SceneSerializer;

// Файлы внутри папки проекта: дерево для панели файлов и операции над ним.
// Все пути - относительно папки проекта. Выйти за её пределы нельзя.
namespace projectfiles {

inline constexpr const char* kSceneExtension = ".yscene";

struct Entry {
    std::string name;
    std::filesystem::path path; // относительно папки проекта
    bool directory = false;
    std::vector<Entry> children;
};

// Дерево файлов: папки первыми, внутри по алфавиту. Файл проекта и скрытые
// файлы (имя с точки) не показываются.
Entry scan(const Project& project);

bool isScene(const std::filesystem::path& path);

// Новая пустая сцена в папке folder: "New Scene.yscene", "New Scene 2.yscene"...
std::optional<std::filesystem::path> createScene(const Project& project, const std::filesystem::path& folder,
                                                 const SceneSerializer& serializer);
std::optional<std::filesystem::path> createFolder(const Project& project, const std::filesystem::path& folder);

// Переименовывает файл или папку в той же папке. Возвращает новый путь;
// nullopt, если имя недопустимо, занято или путь трогать нельзя.
std::optional<std::filesystem::path> rename(const Project& project, const std::filesystem::path& path,
                                            const std::string& newName);

// Удаляет файл или папку со всем содержимым.
bool remove(const Project& project, const std::filesystem::path& path);

// Путь указывает внутрь проекта и это не сам файл проекта и не корень папки.
bool canModify(const Project& project, const std::filesystem::path& path);

} // namespace projectfiles

} // namespace yqlora
