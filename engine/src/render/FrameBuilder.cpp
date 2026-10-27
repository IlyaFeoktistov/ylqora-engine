#include "yqlora/render/FrameBuilder.hpp"

#include "yqlora/render/MeshData.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/MeshRenderer.hpp"
#include "yqlora/scene/Scene.hpp"

namespace yqlora {

namespace {

void collectDrawItems(const Node& node, std::vector<DrawItem>& out)
{
    const auto* node3D = dynamic_cast<const Node3D*>(&node);
    const MeshRenderer* renderer = node3D ? node3D->getComponent<MeshRenderer>() : nullptr;
    if (const MeshData* mesh = renderer ? findBuiltinMesh(renderer->mesh().id) : nullptr) {
        out.push_back({
            .mesh = mesh,
            .world = node3D->worldMatrix(),
            .color = renderer->color(),
            .node = node3D->id(),
        });
    }
    for (const auto& child : node.children()) {
        collectDrawItems(*child, out);
    }
}

} // namespace

FrameData buildFrameData(const Scene& scene, float aspect, DepthRange depthRange)
{
    FrameData frame;
    frame.clearColor = scene.clearColor();
    if (const Camera3D* camera = scene.activeCamera()) {
        frame.camera.view = camera->viewMatrix();
        frame.camera.projection = camera->projectionMatrix(aspect, depthRange);
        frame.camera.position = camera->worldPosition();
    }
    if (const Node* root = scene.root()) {
        collectDrawItems(*root, frame.drawItems);
    }

    return frame;
}

} // namespace yqlora
