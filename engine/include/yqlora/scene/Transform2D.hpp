#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Component.hpp"

#include <cstdint>

namespace yqlora {

// Положение, поворот и масштаб узла Node2D относительно родителя.
// Встроенный компонент: создаётся вместе с Node2D и снять его нельзя.
//
// +X вправо, +Y вверх; положительный угол поворачивает против часовой стрелки.
class YQLORA_API Transform2D final : public Component {
public:
    std::string_view typeName() const override
    {
        return "Transform2D";
    }

    const Vec2& position() const
    {
        return m_position;
    }

    // в радианах
    float rotation() const
    {
        return m_rotation;
    }

    const Vec2& scale() const
    {
        return m_scale;
    }

    void setPosition(const Vec2& position);
    void setRotation(float radians);
    void setScale(const Vec2& scale);

    void setRotationDegrees(float degrees);
    float rotationDegrees() const;

    // Матрица T * R * S в пространстве родителя.
    Mat3 matrix() const;

private:
    friend class Node2D;
    Transform2D() = default;

    // Растёт при каждом изменении, по нему узел понимает, что кэш устарел.
    std::uint32_t version() const
    {
        return m_version;
    }

    void touch()
    {
        ++m_version;
    }

    Vec2 m_position{0.0f};
    float m_rotation = 0.0f;
    Vec2 m_scale{1.0f};
    std::uint32_t m_version = 0;
};

} // namespace yqlora
