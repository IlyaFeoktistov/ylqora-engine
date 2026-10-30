#include "yqlora/render/RenderTypes.hpp"

#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/render/Camera3D.hpp"
#include "yqlora/render/MeshRenderer.hpp"
#include "yqlora/render/RenderSystem.hpp"

namespace yqlora {

void registerRenderTypes(TypeRegistry& registry)
{
    registry.registerNode<Camera3D>()
        .property("fov", &Camera3D::fovDegrees, &Camera3D::setFovDegrees, PropertyHint::Angle)
        .property("near", &Camera3D::nearPlane, &Camera3D::setNearPlane)
        .property("far", &Camera3D::farPlane, &Camera3D::setFarPlane)
        .property(kCameraOrderProperty, &Camera3D::order, &Camera3D::setOrder)
        .property("cullingMask", &Camera3D::cullingMask, &Camera3D::setCullingMask, PropertyHint::Layers)
        .property("clear", &Camera3D::clear, &Camera3D::setClear)
        .enumNames({"ColorAndDepth", "DepthOnly", "Nothing"})
        .property("background", &Camera3D::background, &Camera3D::setBackground, PropertyHint::Color);

    registry.registerComponent<MeshRenderer>()
        .requiresNode<Node3D>()
        .property("mesh", &MeshRenderer::mesh, &MeshRenderer::setMesh, PropertyHint::Mesh)
        .property("color", &MeshRenderer::color, &MeshRenderer::setColor, PropertyHint::Color);

    registry.registerSystem<RenderSystem>();
}

} // namespace yqlora
