#pragma once

#include "yqlora/scene/Component.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>

namespace yqlora {

// Положение, поворот и масштаб узла Node3D относительно родителя.
// Встроенный компонент: создаётся вместе с Node3D и снять его нельзя.
// Мировую матрицу из него собирает узел — сам Transform про дерево не знает.
//
// Система координат правая: +X вправо, +Y вверх, взгляд вдоль -Z.
class Transform3D final : public Component {
public:
    std::string_view typeName() const override { return "Transform3D"; }
    bool removable() const override { return false; }

    const glm::vec3& position() const { return m_position; }
    const glm::quat& rotation() const { return m_rotation; }
    const glm::vec3& scale() const { return m_scale; }

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setScale(const glm::vec3& scale);

    // Углы Эйлера в градусах (pitch по X, yaw по Y, roll по Z).
    void setRotationDegrees(const glm::vec3& eulerDegrees);
    glm::vec3 rotationDegrees() const;

    // Поворачивает так, чтобы -Z смотрел на target (в координатах родителя).
    void lookAt(const glm::vec3& target, const glm::vec3& up = {0.0f, 1.0f, 0.0f});

    // Матрица T * R * S в пространстве родителя.
    glm::mat4 matrix() const;

    // Растёт при каждом изменении — по нему узел понимает, что кэш устарел.
    std::uint32_t version() const { return m_version; }

private:
    friend class Node3D;
    Transform3D() = default;

    void touch() { ++m_version; }

    glm::vec3 m_position{0.0f};
    glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 m_scale{1.0f};
    std::uint32_t m_version = 0;
};

} // namespace yqlora
