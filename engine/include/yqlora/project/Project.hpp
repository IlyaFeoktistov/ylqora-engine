#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace yqlora {

// Проект - папка с файлом project.yqlora, сценами и ресурсами игры. Нужен и
// редактору, и запуску игры без редактора. Здесь только формат файла проекта;
// проверка имени, стартовая сцена и удаление папки - дело редактора.
class YQLORA_API Project {
public:
    static constexpr std::string_view fileName()
    {
        return "project.yqlora";
    }

    // Читает файл проекта и обходит его ресурсы (см. ResourceDatabase::scan).
    // nullopt, если файла нет или он испорчен.
    static std::optional<Project> open(const std::filesystem::path& directory);

    // Пишет файл нового проекта в существующую папку. nullopt, если записать не удалось.
    static std::optional<Project> create(const std::filesystem::path& directory, std::string name);

    // Только имя из файла проекта, без обхода ресурсов: для списка проектов.
    static std::optional<std::string> readName(const std::filesystem::path& directory);

    bool save() const;

    const std::string& name() const
    {
        return m_name;
    }

    const std::filesystem::path& directory() const
    {
        return m_directory;
    }

    std::filesystem::path projectFile() const
    {
        return m_directory / fileName();
    }

    // Сцена, которая открывается вместе с проектом и запускается в игре.
    ResourceId mainScene() const
    {
        return m_mainScene;
    }

    void setMainScene(ResourceId scene)
    {
        m_mainScene = scene;
    }

    // Путь к главной сцене от папки проекта. nullopt, если её нет.
    std::optional<std::filesystem::path> mainScenePath() const;

    ResourceDatabase& resources()
    {
        return m_resources;
    }

    const ResourceDatabase& resources() const
    {
        return m_resources;
    }

private:
    std::string m_name;
    std::filesystem::path m_directory;
    ResourceId m_mainScene = 0;
    ResourceDatabase m_resources;
};

} // namespace yqlora
