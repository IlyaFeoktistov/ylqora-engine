#include "yqlora/scene/Transform2D.hpp"

#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/trigonometric.hpp>

namespace yqlora {

void Transform2D::setPosition(const glm::vec2& position)
{
    m_position = position;
    touch();
}

void Transform2D::setRotation(float radians)
{
    m_rotation = radians;
    touch();
}

void Transform2D::setScale(const glm::vec2& scale)
{
    m_scale = scale;
    touch();
}

void Transform2D::setRotationDegrees(float degrees)
{
    setRotation(glm::radians(degrees));
}

float Transform2D::rotationDegrees() const
{
    return glm::degrees(m_rotation);
}

glm::mat3 Transform2D::matrix() const
{
    glm::mat3 m = glm::translate(glm::mat3(1.0f), m_position);
    m = glm::rotate(m, m_rotation);
    m = glm::scale(m, m_scale);
    return m;
}

} // namespace yqlora
