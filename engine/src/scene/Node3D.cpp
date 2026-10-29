#include "yqlora/scene/Node3D.hpp"

#include "yqlora/math/Euler.hpp"

#include <glm/gtx/matrix_decompose.hpp>

namespace yqlora {

Node3D::Node3D(std::string name)
    : Node(std::move(name))
{
    // Конструктор Transform3D закрыт, make_unique до него не дотянется.
    m_transform = static_cast<Transform3D*>(&addBuiltinComponent(std::unique_ptr<Transform3D>(new Transform3D())));
}

const Mat4& Node3D::worldMatrix() const
{
    if (m_worldDirty) {
        const Node3D* p = parent3D();
        m_world = p ? p->worldMatrix() * m_transform->matrix() : m_transform->matrix();
        m_worldDirty = false;
    }

    return m_world;
}

void Node3D::markWorldDirty()
{
    if (m_worldDirty) {
        return;
    }
    m_worldDirty = true;
    for (const auto& child : children()) {
        if (auto* child3D = dynamic_cast<Node3D*>(child.get())) {
            child3D->markWorldDirty();
        }
    }
}

Vec3 Node3D::worldPosition() const
{
    return Vec3(worldMatrix()[3]);
}

void Node3D::setWorldPosition(const Vec3& position)
{
    const Node3D* p = parent3D();
    const Vec3 local = p ? Vec3(glm::inverse(p->worldMatrix()) * Vec4(position, 1.0f)) : position;
    m_transform->setPosition(local);
}

std::optional<TransformValues> Node3D::localValuesForWorld(const Mat4& world) const
{
    const Node3D* p = parent3D();
    const Mat4 local = p ? glm::inverse(p->worldMatrix()) * world : world;

    Vec3 scale;
    Quat rotation;
    Vec3 translation;
    Vec3 skew;
    Vec4 perspective;
    if (!glm::decompose(local, scale, rotation, translation, skew, perspective)) {
        return std::nullopt;
    }

    return TransformValues{
        .position = translation,
        .rotationDegrees = math::closestEulerDegrees(rotation, m_transform->rotationDegrees()),
        .scale = scale,
    };
}

void Node3D::setWorldMatrix(const Mat4& world)
{
    if (const auto values = localValuesForWorld(world)) {
        m_transform->setValues(*values);
    }
}

Vec3 Node3D::worldForward() const
{
    return glm::normalize(-Vec3(worldMatrix()[2]));
}

Vec3 Node3D::worldRight() const
{
    return glm::normalize(Vec3(worldMatrix()[0]));
}

Vec3 Node3D::worldUp() const
{
    return glm::normalize(Vec3(worldMatrix()[1]));
}

Vec3 Node3D::toWorld(const Vec3& localPoint) const
{
    return Vec3(worldMatrix() * Vec4(localPoint, 1.0f));
}

Vec3 Node3D::toLocal(const Vec3& worldPoint) const
{
    return Vec3(glm::inverse(worldMatrix()) * Vec4(worldPoint, 1.0f));
}

void Node3D::onParentChanged()
{
    markWorldDirty();
}

std::function<void()> Node3D::captureWorldPosition()
{
    return [this, world = worldMatrix()] {
        setWorldMatrix(world);
    };
}

const Node3D* Node3D::parent3D() const
{
    return dynamic_cast<const Node3D*>(parent());
}

} // namespace yqlora
