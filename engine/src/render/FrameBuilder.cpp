#include "yqlora/render/FrameBuilder.hpp"

#include "yqlora/render/MeshData.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/MeshRenderer.hpp"
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
    for (const MeshRenderer* renderer : scene.components<MeshRenderer>()) {
        if (!renderer->activeAndEnabled()) {
            continue;
        }
        const auto* node = dynamic_cast<const Node3D*>(renderer->owner());
        const MeshData* mesh = findBuiltinMesh(renderer->mesh().id);
        if (node && mesh) {
            frame.drawItems.push_back({
                .mesh = mesh,
                .world = node->worldMatrix(),
                .color = renderer->color(),
                .node = node->id(),
            });
        }
    }

    return frame;
}

} // namespace yqlora
