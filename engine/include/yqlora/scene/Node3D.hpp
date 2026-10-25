#pragma once

#include "yqlora/scene/Node.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace yqlora {

// Узел с положением в 3D-пространстве. Сам по себе — «пустой объект»:
// ничего не рисует, но может быть точкой привязки для детей.
//
// Система координат правая: +X вправо, +Y вверх, взгляд вдоль -Z.
// Transform задаётся относительно ближайшего родителя-Node3D; если родитель —
// обычный Node, узел считается корневым в пространстве мира.
class Node3D : public Node {
public:
    explicit Node3D(std::string name = "Node3D");

    std::string_view typeName() const override { return "Node3D"; }

    const glm::vec3& position() const { return m_position; }
    const glm::quat& rotation() const { return m_rotation; }
    const glm::vec3& scale() const { return m_scale; }

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setScale(const glm::vec3& scale);

    // Углы Эйлера в градусах (pitch по X, yaw по Y, roll по Z).
    void setRotationDegrees(const glm::vec3& eulerDegrees);
    glm::vec3 rotationDegrees() const;

    // Поворачивает узел так, чтобы -Z смотрел на target (в координатах родителя).
    void lookAt(const glm::vec3& target, const glm::vec3& up = {0.0f, 1.0f, 0.0f});

    glm::mat4 localMatrix() const;
    const glm::mat4& worldMatrix() const;
    glm::vec3 worldPosition() const;

    // Направления в мировом пространстве.
    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

protected:
    void onParentChanged() override;

private:
    void markWorldDirty();
    const Node3D* parent3D() const;

    glm::vec3 m_position{0.0f};
    glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 m_scale{1.0f};

    mutable glm::mat4 m_world{1.0f};
    mutable bool m_worldDirty = true;
};

} // namespace yqlora
