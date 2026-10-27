#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/Transform2D.hpp"

#include <cstdint>

namespace yqlora {

// Базовый узел 2D-сцены. Всегда имеет Transform2D; сам по себе ничего
// не рисует, но может быть точкой привязки для детей.
//
// Transform задаётся относительно родителя-Node2D; если родитель не Node2D,
// узел считается корневым в пространстве мира.
class YQLORA_API Node2D : public Node {
public:
    explicit Node2D(std::string name = "Node2D");

    YQLORA_TYPE(Node2D)

    Transform2D& transform()
    {
        return *m_transform;
    }

    const Transform2D& transform() const
    {
        return *m_transform;
    }

    const Mat3& worldMatrix() const;
    Vec2 worldPosition() const;

protected:
    void onParentChanged() override;

private:
    const Node2D* parent2D() const;

    Transform2D* m_transform = nullptr; // принадлежит списку компонентов узла

    // Кэш мировой матрицы, как в Node3D.
    mutable Mat3 m_world{1.0f};
    mutable std::uint32_t m_worldVersion = 0;
    mutable std::uint32_t m_seenLocalVersion = 0;
    mutable std::uint32_t m_seenParentVersion = 0;
    mutable bool m_worldValid = false;
};

} // namespace yqlora
