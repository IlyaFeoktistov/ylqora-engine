#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace yqlora {

class SceneSerializer;

enum class ProjectCompatibility {
    Current,
    // Старый формат, который можно перевести на текущий (см. Project::upgrade).
    Older,
    // Сохранён более новой версией движка.
    Newer,
    // Без версии или с версией, перевода с которой нет.
    Unsupported,
};

// Что известно о проекте без его открытия: для списка проектов.
struct ProjectInfo {
    std::string name;
    int formatVersion = 0;
    // Версия движка, который сохранил проект последним. Пусто, если неизвестно.
    std::string engineVersion;
    ProjectCompatibility compatibility = ProjectCompatibility::Unsupported;
};

// Проект - папка с файлом project.yqlora, сценами и ресурсами игры. Нужен и
// редактору, и запуску игры без редактора. Здесь только формат файла проекта;
// проверка имени, стартовая сцена и удаление папки - дело редактора.
class YQLORA_API Project {
public:
    static constexpr std::string_view fileName()
    {
        return "project.yqlora";
    }

    // Версия формата проекта. Растёт при любой смене формата проекта или сцены.
    static int formatVersion();

    // Читает файл проекта и обходит его ресурсы (см. ResourceDatabase::scan).
    // nullopt, если файла нет, он испорчен или его формат не текущий.
    static std::optional<Project> open(const std::filesystem::path& directory);

    // Пишет файл нового проекта в существующую папку. nullopt, если записать не удалось.
    static std::optional<Project> create(const std::filesystem::path& directory, std::string name);

    // Сведения из файла проекта, без обхода ресурсов. nullopt, если это не проект.
    static std::optional<ProjectInfo> readInfo(const std::filesystem::path& directory);

    // Переводит проект старого формата на текущий: файл проекта и все сцены.
    // После этого старые версии движка его не откроют. Сцены сначала читаются
    // все, и только если все прочитались, записываются: при ошибке проект
    // остаётся как был. serializer должен знать типы игры, как при открытии.
    static bool upgrade(const std::filesystem::path& directory, const SceneSerializer& serializer);

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
