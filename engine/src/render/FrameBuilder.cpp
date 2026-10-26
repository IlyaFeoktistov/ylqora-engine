#include "yqlora/render/FrameBuilder.hpp"

#include "yqlora/render/MeshData.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/MeshInstance3D.hpp"
#include "yqlora/scene/Scene.hpp"

namespace yqlora {

namespace {

void collectDrawItems(const Node& node, std::vector<DrawItem>& out)
{
    if (const auto* instance = dynamic_cast<const MeshInstance3D*>(&node)) {
        if (const MeshData* mesh = findBuiltinMesh(instance->mesh())) {
            out.push_back({
                .mesh = mesh,
                .world = instance->worldMatrix(),
                .color = instance->color(),
                .node = instance->id(),
            });
        }
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
