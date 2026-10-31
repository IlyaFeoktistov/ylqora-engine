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

// Создаёт папку location/name с файлом проекта и стартовыми папками: scenes/
// со стартовой сценой (камера и куб), scripts/ и assets/. Папки - только
// порядок для начала: ресурсы и скрипты могут лежать где угодно.
// При ошибке возвращает nullopt и пишет причину в error: это ключ перевода,
// его можно сразу показать через tr().
std::optional<Project> createProject(
    const std::filesystem::path& location,
    const std::string& name,
    const SceneSerializer& serializer,
    std::string& error
);

// Удаляет папку проекта с диска со всем содержимым. Только если в ней
// действительно лежит файл проекта: так случайно не стереть чужую папку.
// При ошибке пишет ключ перевода в error.
bool deleteProjectFolder(const std::filesystem::path& directory, std::string& error);

// Имя годится и как название проекта, и как имя файла или папки на любой системе.
bool isValidFileName(std::string_view name);

} // namespace yqlora::editor
