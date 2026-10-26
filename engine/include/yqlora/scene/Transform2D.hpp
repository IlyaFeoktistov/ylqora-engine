#pragma once

#include "yqlora/scene/Component.hpp"

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

#include <cstdint>

namespace yqlora {

// Положение, поворот и масштаб узла Node2D относительно родителя.
// Встроенный компонент: создаётся вместе с Node2D и снять его нельзя.
//
// +X вправо, +Y вверх; положительный угол поворачивает против часовой стрелки.
class Transform2D final : public Component {
public:
    std::string_view typeName() const override { return "Transform2D"; }
    bool removable() const override { return false; }

    const glm::vec2& position() const { return m_position; }
    float rotation() const { return m_rotation; } // в радианах
    const glm::vec2& scale() const { return m_scale; }

    void setPosition(const glm::vec2& position);
    void setRotation(float radians);
    void setScale(const glm::vec2& scale);

    void setRotationDegrees(float degrees);
    float rotationDegrees() const;

    // Матрица T * R * S в пространстве родителя.
    glm::mat3 matrix() const;

    // Растёт при каждом изменении — по нему узел понимает, что кэш устарел.
    std::uint32_t version() const { return m_version; }

private:
    friend class Node2D;
    Transform2D() = default;

    void touch() { ++m_version; }

    glm::vec2 m_position{0.0f};
    float m_rotation = 0.0f;
    glm::vec2 m_scale{1.0f};
    std::uint32_t m_version = 0;
};

} // namespace yqlora
