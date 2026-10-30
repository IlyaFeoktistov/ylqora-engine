#pragma once

#include "yqlora/resource/ResourceId.hpp"
#include "yqlora/scene/Scene.hpp"

#include <filesystem>
#include <string>

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

    // Отличается ли сцена от своего файла: как бы её ни поменяли, через
    // историю или нет. Значение с последнего refreshModified: сравнение -
    // запись всей сцены, для каждого кадра дорого.
    bool modified() const
    {
        return m_modified;
    }

    void refreshModified();

private:
    Scene m_scene;
    const SceneSerializer& m_serializer;
    ResourceId m_id = 0;
    // Сцена, как она записана в файле (или пустая после reset).
    std::string m_savedText;
    bool m_modified = false;
};

} // namespace yqlora::editor
