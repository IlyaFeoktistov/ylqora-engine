#include "yqlora/render/FrameBuilder.hpp"

#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"

namespace yqlora {

FrameData buildFrameData(const Scene& scene, float aspect, DepthRange depthRange)
{
    FrameData frame;
    frame.clearColor = scene.clearColor();
    if (const Camera3D* camera = scene.activeCamera()) {
        frame.camera.view = camera->viewMatrix();
        frame.camera.projection = camera->projectionMatrix(aspect, depthRange);
        frame.camera.position = camera->worldPosition();
    }

    return frame;
}

} // namespace yqlora
