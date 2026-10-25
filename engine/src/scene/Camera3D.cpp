#include "yqlora/scene/Camera3D.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cassert>

namespace yqlora {

Camera3D::Camera3D(std::string name)
    : Node3D(std::move(name))
{
}

void Camera3D::setFovDegrees(float fov)
{
    m_fovDegrees = std::clamp(fov, 1.0f, 179.0f);
}

void Camera3D::setClipPlanes(float nearPlane, float farPlane)
{
    assert(nearPlane > 0.0f && farPlane > nearPlane);
    m_near = nearPlane;
    m_far = farPlane;
}

glm::mat4 Camera3D::viewMatrix() const
{
    // View — обратное преобразование к положению камеры в мире.
    return glm::inverse(worldMatrix());
}

glm::mat4 Camera3D::projectionMatrix(float aspect, DepthRange depthRange) const
{
    const float fovY = glm::radians(m_fovDegrees);
    if (depthRange == DepthRange::ZeroToOne) {
        return glm::perspectiveRH_ZO(fovY, aspect, m_near, m_far);
    }
    return glm::perspectiveRH_NO(fovY, aspect, m_near, m_far);
}

} // namespace yqlora
