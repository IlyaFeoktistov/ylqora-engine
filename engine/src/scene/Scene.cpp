#include "yqlora/scene/Scene.hpp"

#include "yqlora/scene/Camera3D.hpp"

namespace yqlora {

Scene::Scene()
    : m_root("Root")
{
}

void Scene::update(float deltaSeconds)
{
    m_root.update(deltaSeconds);
}

FrameData Scene::buildFrameData(float aspect, DepthRange depthRange) const
{
    FrameData frame;
    frame.clearColor = m_clearColor;
    if (m_activeCamera) {
        frame.camera.view = m_activeCamera->viewMatrix();
        frame.camera.projection = m_activeCamera->projectionMatrix(aspect, depthRange);
        frame.camera.position = m_activeCamera->worldPosition();
    }
    return frame;
}

} // namespace yqlora
