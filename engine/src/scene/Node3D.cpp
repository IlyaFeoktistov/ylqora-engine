#include "yqlora/scene/Node3D.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace yqlora {

Node3D::Node3D(std::string name)
    : Node(std::move(name))
{
}

void Node3D::setPosition(const glm::vec3& position)
{
    m_position = position;
    markWorldDirty();
}

void Node3D::setRotation(const glm::quat& rotation)
{
    m_rotation = glm::normalize(rotation);
    markWorldDirty();
}

void Node3D::setScale(const glm::vec3& scale)
{
    m_scale = scale;
    markWorldDirty();
}

void Node3D::setRotationDegrees(const glm::vec3& eulerDegrees)
{
    setRotation(glm::quat(glm::radians(eulerDegrees)));
}

glm::vec3 Node3D::rotationDegrees() const
{
    return glm::degrees(glm::eulerAngles(m_rotation));
}

void Node3D::lookAt(const glm::vec3& target, const glm::vec3& up)
{
    const glm::vec3 direction = target - m_position;
    if (glm::length(direction) < 1e-6f) {
        return;
    }
    setRotation(glm::quatLookAt(glm::normalize(direction), up));
}

glm::mat4 Node3D::localMatrix() const
{
    glm::mat4 m = glm::translate(glm::mat4(1.0f), m_position);
    m *= glm::toMat4(m_rotation);
    m = glm::scale(m, m_scale);
    return m;
}

const glm::mat4& Node3D::worldMatrix() const
{
    if (m_worldDirty) {
        const Node3D* p = parent3D();
        m_world = p ? p->worldMatrix() * localMatrix() : localMatrix();
        m_worldDirty = false;
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
    markWorldDirty();
}

void Node3D::markWorldDirty()
{
    if (m_worldDirty) {
        return; // потомки уже помечены, когда помечали нас
    }
    m_worldDirty = true;
    for (const auto& child : children()) {
        if (auto* child3D = dynamic_cast<Node3D*>(child.get())) {
            child3D->markWorldDirty();
        }
    }
}

const Node3D* Node3D::parent3D() const
{
    return dynamic_cast<const Node3D*>(parent());
}

} // namespace yqlora
