#include "yqlora/render/FrameBuilder.hpp"

#include "yqlora/render/BuiltinMeshes.hpp"
#include "yqlora/render/Camera3D.hpp"
#include "yqlora/render/MeshRenderer.hpp"
#include "yqlora/render/RenderSystem.hpp"
#include "yqlora/scene/Scene.hpp"

namespace yqlora {

FrameData extractFrame(const Scene& scene, float aspect, DepthRange depthRange)
{
    FrameData frame;
    if (const RenderSystem* render = scene.system<RenderSystem>()) {
        for (const Camera3D* camera : render->cameras()) {
            frame.views.push_back(cameraView(*camera, aspect, depthRange));
        }
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
                .layer = node->layer(),
                .node = node->id(),
            });
        }
    }

    return frame;
}

ViewData cameraView(const Camera3D& camera, float aspect, DepthRange depthRange)
{
    return {
        .camera =
            {
                .view = camera.viewMatrix(),
                .projection = camera.projectionMatrix(aspect, depthRange),
                .position = camera.worldPosition(),
            },
        .clear = camera.clear(),
        .background = camera.background(),
        .mask = camera.cullingMask(),
    };
}

} // namespace yqlora
