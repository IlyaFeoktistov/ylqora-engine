#pragma once

#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/Transform3D.hpp"

#include <cstdint>

namespace yqlora {

// Базовый узел 3D-сцены. Всегда имеет Transform3D. Сам по себе это пустой
// объект: ничего не рисует, но может быть точкой привязки для детей.
//
// Transform задаётся относительно родителя-Node3D; если родитель не Node3D,
// узел считается корневым в пространстве мира.
class Node3D : public Node {
public:
    explicit Node3D(std::string name = "Node3D");

    std::string_view typeName() const override
    {
        return "Node3D";
    }

    Transform3D& transform()
    {
        return *m_transform;
    }

    const Transform3D& transform() const
    {
        return *m_transform;
    }

    const Mat4& worldMatrix() const;
    Vec3 worldPosition() const;

    // Направления в мировом пространстве.
    Vec3 forward() const;
    Vec3 right() const;
    Vec3 up() const;

protected:
    void onParentChanged() override;

private:
    const Node3D* parent3D() const;

    Transform3D* m_transform = nullptr; // принадлежит списку компонентов узла

    // Кэш мировой матрицы. Валиден, пока не изменились ни свой Transform,
    // ни мировая матрица родителя (сверяем по версиям).
    mutable Mat4 m_world{1.0f};
    mutable std::uint32_t m_worldVersion = 0;
    mutable std::uint32_t m_seenLocalVersion = 0;
    mutable std::uint32_t m_seenParentVersion = 0;
    mutable bool m_worldValid = false;
};

} // namespace yqlora
