#include "yqlora/scene/Node2D.hpp"

namespace yqlora {

Node2D::Node2D(std::string name)
    : Node(std::move(name))
{
    // Конструктор Transform2D закрыт — make_unique до него не дотянется.
    m_transform = static_cast<Transform2D*>(&addComponent(std::unique_ptr<Transform2D>(new Transform2D())));
}

const glm::mat3& Node2D::worldMatrix() const
{
    // Сначала актуализируем родителя: его версия говорит, менялся ли он.
    const Node2D* p = parent2D();
    const glm::mat3* parentWorld = p ? &p->worldMatrix() : nullptr;
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

glm::vec2 Node2D::worldPosition() const
{
    return glm::vec2(worldMatrix()[2]);
}

void Node2D::onParentChanged()
{
    // Новый родитель может случайно иметь ту же версию, что и старый.
    m_worldValid = false;
}

const Node2D* Node2D::parent2D() const
{
    return dynamic_cast<const Node2D*>(parent());
}

} // namespace yqlora
