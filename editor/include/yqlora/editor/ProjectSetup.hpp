#pragma once

#include "yqlora/project/Project.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace yqlora {

class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

// Шаблон папки scripts/ среди ресурсов редактора (assets).
std::filesystem::path scriptsTemplateDirectory(const std::filesystem::path& assets);

// Создаёт папку location/name с файлом проекта, стартовой сценой (камера и куб)
// и папкой scripts/ из шаблона scriptsTemplate.
// При ошибке возвращает nullopt и пишет причину в error: это ключ перевода,
// его можно сразу показать через tr().
std::optional<Project> createProject(
    const std::filesystem::path& location,
    const std::string& name,
    const SceneSerializer& serializer,
    const std::filesystem::path& scriptsTemplate,
    std::string& error
);

// Копирует шаблон в папку scripts/ проекта, у которого её ещё нет (проект
// создан до шага 3 или папку удалили). Существующую не трогает: там чужой код.
// При ошибке пишет ключ перевода в error.
bool createScriptsFolder(
    const std::filesystem::path& projectDirectory,
    const std::filesystem::path& scriptsTemplate,
    std::string& error
);

// Удаляет папку проекта с диска со всем содержимым. Только если в ней
// действительно лежит файл проекта: так случайно не стереть чужую папку.
// При ошибке пишет ключ перевода в error.
bool deleteProjectFolder(const std::filesystem::path& directory, std::string& error);

// Имя годится и как название проекта, и как имя файла или папки на любой системе.
bool isValidFileName(std::string_view name);

} // namespace yqlora::editor
