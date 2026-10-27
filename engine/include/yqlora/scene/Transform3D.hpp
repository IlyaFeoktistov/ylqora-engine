#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Component.hpp"

#include <cstdint>

namespace yqlora {

// Положение, поворот и масштаб узла Node3D относительно родителя.
// Встроенный компонент: создаётся вместе с Node3D и снять его нельзя.
// Мировую матрицу из него собирает узел, сам Transform про дерево не знает.
//
// Система координат правая: +X вправо, +Y вверх, взгляд вдоль -Z.
class YQLORA_API Transform3D final : public Component {
public:
    YQLORA_TYPE(Transform3D)

    const Vec3& position() const
    {
        return m_position;
    }

    const Quat& rotation() const
    {
        return m_rotation;
    }

    const Vec3& scale() const
    {
        return m_scale;
    }

    void setPosition(const Vec3& position);
    void setRotation(const Quat& rotation);
    void setScale(const Vec3& scale);

    // Углы Эйлера в градусах (pitch по X, yaw по Y, roll по Z).
    // Возвращаются ровно такими, как заданы: из кватерниона углы восстанавливаются
    // неоднозначно (yaw 100 градусов превращается в (180, 80, 180)), и поле в инспекторе прыгало бы.
    // После setRotation/lookAt углы вычисляются из кватерниона.
    void setRotationDegrees(const Vec3& eulerDegrees);
    const Vec3& rotationDegrees() const
    {
        return m_rotationDegrees;
    }

    // Поворачивает так, чтобы -Z смотрел на target (в координатах родителя).
    void lookAt(const Vec3& target, const Vec3& up = {0.0f, 1.0f, 0.0f});

    // Матрица T * R * S в пространстве родителя.
    Mat4 matrix() const;

private:
    friend class Node3D;
    Transform3D() = default;

    // Растёт при каждом изменении, по нему узел понимает, что кэш устарел.
    std::uint32_t version() const
    {
        return m_version;
    }

    void touch()
    {
        ++m_version;
    }

    Vec3 m_position{0.0f};
    Quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 m_rotationDegrees{0.0f};
    Vec3 m_scale{1.0f};
    std::uint32_t m_version = 0;
};

} // namespace yqlora
