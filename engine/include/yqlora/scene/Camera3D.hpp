#pragma once

#include "yqlora/render/FrameData.hpp"
#include "yqlora/scene/Node3D.hpp"

namespace yqlora {

// Перспективная камера. Смотрит вдоль своей оси -Z.
// Соотношение сторон — свойство области вывода, а не камеры,
// поэтому передаётся при построении проекции.
class Camera3D : public Node3D {
public:
    explicit Camera3D(std::string name = "Camera3D");

    std::string_view typeName() const override { return "Camera3D"; }

    float fovDegrees() const { return m_fovDegrees; } // вертикальный угол обзора
    float nearPlane() const { return m_near; }
    float farPlane() const { return m_far; }

    void setFovDegrees(float fov);
    void setClipPlanes(float nearPlane, float farPlane);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect, DepthRange depthRange) const;

private:
    float m_fovDegrees = 60.0f;
    float m_near = 0.1f;
    float m_far = 1000.0f;
};

} // namespace yqlora
