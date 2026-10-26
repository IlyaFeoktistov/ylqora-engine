#include "yqlora/reflection/TypeRegistry.hpp"

#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/MeshInstance3D.hpp"
#include "yqlora/scene/Node3D.hpp"
#include "yqlora/scene/Transform3D.hpp"

#include <ranges>

namespace yqlora {

std::unique_ptr<Node> TypeInfo::createNode() const
{
    return m_createNode ? m_createNode() : nullptr;
}

std::unique_ptr<Component> TypeInfo::createComponent() const
{
    return m_createComponent ? m_createComponent() : nullptr;
}

const TypeInfo* TypeRegistry::find(std::string_view name) const
{
    const auto it = m_byName.find(name);

    return it != m_byName.end() ? it->second : nullptr;
}

std::vector<const Property*> TypeRegistry::properties(const TypeInfo& type) const
{
    // Собираем цепочку от типа к корню, потом идём обратно: базовые свойства первыми.
    std::vector<const TypeInfo*> chain;
    for (const TypeInfo* t = &type; t; t = t->baseName().empty() ? nullptr : find(t->baseName())) {
        chain.push_back(t);
    }

    std::vector<const Property*> result;
    for (const TypeInfo* t : chain | std::views::reverse) {
        for (const Property& property : t->ownProperties()) {
            result.push_back(&property);
        }
    }

    return result;
}

void registerEngineTypes(TypeRegistry& registry)
{
    registry.registerNode<Node>("Node")
        .property("name", &Node::name, &Node::setName);

    registry.registerNode<Node3D>("Node3D");

    registry.registerNode<Camera3D, Node3D>("Camera3D")
        .property("fov", &Camera3D::fovDegrees, &Camera3D::setFovDegrees)
        .property("near", &Camera3D::nearPlane, &Camera3D::setNearPlane)
        .property("far", &Camera3D::farPlane, &Camera3D::setFarPlane);

    registry.registerNode<MeshInstance3D, Node3D>("MeshInstance3D")
        .property("mesh", &MeshInstance3D::mesh, &MeshInstance3D::setMesh)
        .property("color", &MeshInstance3D::color, &MeshInstance3D::setColor);

    registry.registerComponent<Transform3D>("Transform3D")
        .property("position", &Transform3D::position, &Transform3D::setPosition)
        .property("rotation", &Transform3D::rotationDegrees, &Transform3D::setRotationDegrees)
        .property("scale", &Transform3D::scale, &Transform3D::setScale);
}

} // namespace yqlora
