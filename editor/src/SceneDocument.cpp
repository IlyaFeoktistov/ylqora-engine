#include "yqlora/editor/SceneDocument.hpp"

#include "yqlora/project/Project.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

namespace yqlora::editor {

SceneDocument::SceneDocument(const EngineServices& services, const SceneSerializer& serializer)
    : m_scene(services)
    , m_serializer(serializer)
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
}

bool SceneDocument::load(const Project& project, ResourceId id)
{
    const auto file = project.resources().path(id);
    if (!file || !m_serializer.load(m_scene, project.directory() / *file)) {
        return false;
    }
    m_id = id;

    return true;
}

bool SceneDocument::save(const Project& project)
{
    const std::filesystem::path file = path(project);

    return !file.empty() && m_serializer.save(m_scene, project.directory() / file);
}

} // namespace yqlora::editor
