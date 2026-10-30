#pragma once

#include "yqlora/resource/ResourceId.hpp"
#include "yqlora/scene/Scene.hpp"

#include <filesystem>

namespace yqlora {

class EngineServices;
class Project;
class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

// Редактируемая сцена как файл проекта: какая открыта, откуда читается и
// куда сохраняется. Историю правок ведёт сессия (EditorSession).
class SceneDocument {
public:
    // Сцена документа создаётся со службами владельца, они её переживают.
    SceneDocument(const EngineServices& services, const SceneSerializer& serializer);

    Scene& scene()
    {
        return m_scene;
    }

    const Scene& scene() const
    {
        return m_scene;
    }

    // Сцена как ресурс проекта: путь к ней меняется при переименовании, а ID нет.
    ResourceId id() const
    {
        return m_id;
    }

    // Путь к файлу сцены от папки проекта. Пусто, если сцены нет.
    std::filesystem::path path(const Project& project) const;

    // Пустая сцена, которая сохранится в ресурс id (0 - никуда).
    void reset(ResourceId id);

    // Читает сцену из ресурса id. Если файл не читается, остаётся прежняя.
    bool load(const Project& project, ResourceId id);

    bool save(const Project& project);

private:
    Scene m_scene;
    const SceneSerializer& m_serializer;
    ResourceId m_id = 0;
};

} // namespace yqlora::editor
