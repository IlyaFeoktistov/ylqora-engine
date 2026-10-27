#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace yqlora {

class SceneSerializer;

// Проект - папка с файлом project.yqlora, сценами и ресурсами игры.
struct Project {
    static constexpr std::string_view kFileName = "project.yqlora";

    std::string name;
    std::filesystem::path directory;
    // Сцена, которая открывается вместе с проектом. Путь от папки проекта.
    std::filesystem::path mainScene = "main.yscene";

    std::filesystem::path projectFile() const
    {
        return directory / kFileName;
    }

    std::filesystem::path mainScenePath() const
    {
        return directory / mainScene;
    }

    // Читает project.yqlora из папки. nullopt, если файла нет или он испорчен.
    static std::optional<Project> load(const std::filesystem::path& directory);

    bool save() const;

    // Создаёт папку location/name с файлом проекта и стартовой сценой
    // (камера и куб). При ошибке возвращает nullopt и пишет причину в error:
    // это ключ перевода, его можно сразу показать через tr().
    static std::optional<Project> create(
        const std::filesystem::path& location,
        const std::string& name,
        const SceneSerializer& serializer,
        std::string& error
    );

    // Удаляет папку проекта с диска со всем содержимым. Только если в ней
    // действительно лежит project.yqlora: так случайно не стереть чужую папку.
    // При ошибке пишет ключ перевода в error.
    static bool deleteFolder(const std::filesystem::path& directory, std::string& error);

    // Имя годится и как название, и как имя папки на любой системе.
    static bool isValidName(std::string_view name);
};

} // namespace yqlora
