#include "yqlora/editor/SceneDocument.hpp"

#include "yqlora/project/Project.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

namespace yqlora::editor {

SceneDocument::SceneDocument(const EngineServices& services, const SceneSerializer& serializer)
    : m_scene(services)
    , m_serializer(serializer)
    , m_savedText(serializer.toJson(m_scene))
{
}

std::filesystem::path SceneDocument::path(const Project& project) const
{
    return project.resources().path(m_id).value_or(std::filesystem::path());
}

void SceneDocument::reset(ResourceId id)
{
    m_scene.setRoot(nullptr);
    m_id = id;
    m_savedText = m_serializer.toJson(m_scene);
    m_modified = false;
}

bool SceneDocument::load(const Project& project, ResourceId id)
{
    const auto file = project.resources().path(id);
    if (!file || !m_serializer.load(m_scene, project.directory() / *file)) {
        return false;
    }
    m_id = id;
    m_savedText = m_serializer.toJson(m_scene);
    m_modified = false;

    return true;
}

bool SceneDocument::save(const Project& project)
{
    const std::filesystem::path file = path(project);

    if (file.empty() || !m_serializer.save(m_scene, project.directory() / file)) {
        return false;
    }
    m_savedText = m_serializer.toJson(m_scene);
    m_modified = false;

    return true;
}

void SceneDocument::refreshModified()
{
    m_modified = m_serializer.toJson(m_scene) != m_savedText;
}

} // namespace yqlora::editor
