#pragma once

#include "yqlora/editor/EditHistory.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <filesystem>

namespace yqlora {

class Project;
class Scene;
class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

// Редактируемая сцена как файл проекта: какая открыта, сохранена ли и история
// правок с момента открытия.
class SceneDocument {
public:
    SceneDocument(Scene& scene, const SceneSerializer& serializer);

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

    // Пустая сцена, которая сохранится в ресурс id (0 - никуда), без истории.
    void reset(ResourceId id);

    // Читает сцену из ресурса id. Если файл не читается, остаётся прежняя.
    bool load(const Project& project, ResourceId id);

    bool save(const Project& project);

    // Сцену меняли после открытия или последнего сохранения.
    bool modified() const
    {
        return !m_history.atSaved();
    }

    EditHistory& history()
    {
        return m_history;
    }

    const EditHistory& history() const
    {
        return m_history;
    }

private:
    Scene& m_scene;
    const SceneSerializer& m_serializer;
    ResourceId m_id = 0;
    EditHistory m_history;
};

} // namespace yqlora::editor
