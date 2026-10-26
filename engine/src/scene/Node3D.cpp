#include "yqlora/scene/Node3D.hpp"

namespace yqlora {

Node3D::Node3D(std::string name)
    : Node(std::move(name))
{
    // Конструктор Transform3D закрыт, make_unique до него не дотянется.
    m_transform = static_cast<Transform3D*>(&addBuiltinComponent(std::unique_ptr<Transform3D>(new Transform3D())));
}

const Mat4& Node3D::worldMatrix() const
{
    // Сначала актуализируем родителя: его версия говорит, менялся ли он.
    const Node3D* p = parent3D();
    const Mat4* parentWorld = p ? &p->worldMatrix() : nullptr;
    const std::uint32_t parentVersion = p ? p->m_worldVersion : 0;

    if (!m_worldValid || m_seenLocalVersion != m_transform->version() || m_seenParentVersion != parentVersion) {
        m_world = parentWorld ? *parentWorld * m_transform->matrix() : m_transform->matrix();
        m_seenLocalVersion = m_transform->version();
        m_seenParentVersion = parentVersion;
        m_worldValid = true;
        ++m_worldVersion;
    }

    return m_world;
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
    // Новый родитель может случайно иметь ту же версию, что и старый.
    m_worldValid = false;
}

const Node3D* Node3D::parent3D() const
{
    return dynamic_cast<const Node3D*>(parent());
}

} // namespace yqlora
