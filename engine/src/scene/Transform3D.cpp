#include "yqlora/scene/Transform3D.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace yqlora {

void Transform3D::setPosition(const glm::vec3& position)
{
    m_position = position;
    touch();
}

void Transform3D::setRotation(const glm::quat& rotation)
{
    m_rotation = glm::normalize(rotation);
    touch();
}

void Transform3D::setScale(const glm::vec3& scale)
{
    m_scale = scale;
    touch();
}

void Transform3D::setRotationDegrees(const glm::vec3& eulerDegrees)
{
    setRotation(glm::quat(glm::radians(eulerDegrees)));
}

glm::vec3 Transform3D::rotationDegrees() const
{
    return glm::degrees(glm::eulerAngles(m_rotation));
}

void Transform3D::lookAt(const glm::vec3& target, const glm::vec3& up)
{
    const glm::vec3 direction = target - m_position;
    if (glm::length(direction) < 1e-6f) {
        return;
    }
    setRotation(glm::quatLookAt(glm::normalize(direction), up));
}

glm::mat4 Transform3D::matrix() const
{
    glm::mat4 m = glm::translate(glm::mat4(1.0f), m_position);
    m *= glm::toMat4(m_rotation);
    m = glm::scale(m, m_scale);
    return m;
}

} // namespace yqlora
