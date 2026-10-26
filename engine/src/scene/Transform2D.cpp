#include "yqlora/scene/Transform2D.hpp"

#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/trigonometric.hpp>

namespace yqlora {

void Transform2D::setPosition(const Vec2& position)
{
    m_position = position;
    touch();
}

void Transform2D::setRotation(float radians)
{
    m_rotation = radians;
    touch();
}

void Transform2D::setScale(const Vec2& scale)
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

Mat3 Transform2D::matrix() const
{
    Mat3 m = glm::translate(Mat3(1.0f), m_position);
    m = glm::rotate(m, m_rotation);
    m = glm::scale(m, m_scale);

    return m;
}

} // namespace yqlora
