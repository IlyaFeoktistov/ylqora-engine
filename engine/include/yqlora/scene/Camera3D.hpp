#pragma once

#include "yqlora/math/DepthRange.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node3D.hpp"

namespace yqlora {

// Перспективная камера. Смотрит вдоль своей оси -Z.
// Соотношение сторон это свойство области вывода, а не камеры,
// поэтому передаётся при построении проекции.
class Camera3D : public Node3D {
public:
    explicit Camera3D(std::string name = "Camera3D");

    std::string_view typeName() const override
    {
        return "Camera3D";
    }

    // вертикальный угол обзора
    float fovDegrees() const
    {
        return m_fovDegrees;
    }

    float nearPlane() const
    {
        return m_near;
    }

    float farPlane() const
    {
        return m_far;
    }

    void setFovDegrees(float fov);
    void setClipPlanes(float nearPlane, float farPlane);

    // По одной плоскости, для инспектора и загрузки сцены. Дальняя плоскость
    // всегда остаётся дальше ближней: при конфликте сдвигается дальняя.
    void setNearPlane(float nearPlane);
    void setFarPlane(float farPlane);

    Mat4 viewMatrix() const;
    Mat4 projectionMatrix(float aspect, DepthRange depthRange) const;

private:
    float m_fovDegrees = 60.0f;
    float m_near = 0.1f;
    float m_far = 1000.0f;
};

} // namespace yqlora
