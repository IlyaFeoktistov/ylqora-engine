#include "yqlora/scene/Node3D.hpp"

namespace yqlora {

Node3D::Node3D(std::string name)
    : Node(std::move(name))
{
    // Конструктор Transform3D закрыт — make_unique до него не дотянется.
    m_transform = static_cast<Transform3D*>(&addComponent(std::unique_ptr<Transform3D>(new Transform3D())));
}

const glm::mat4& Node3D::worldMatrix() const
{
    // Сначала актуализируем родителя: его версия говорит, менялся ли он.
    const Node3D* p = parent3D();
    const glm::mat4* parentWorld = p ? &p->worldMatrix() : nullptr;
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

glm::vec3 Node3D::worldPosition() const
{
    return glm::vec3(worldMatrix()[3]);
}

glm::vec3 Node3D::forward() const
{
    return glm::normalize(-glm::vec3(worldMatrix()[2]));
}

glm::vec3 Node3D::right() const
{
    return glm::normalize(glm::vec3(worldMatrix()[0]));
}

glm::vec3 Node3D::up() const
{
    return glm::normalize(glm::vec3(worldMatrix()[1]));
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
