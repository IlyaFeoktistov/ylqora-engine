#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/Transform3D.hpp"

#include <functional>
#include <optional>

namespace yqlora {

class TransformSystem3D;

// Базовый узел 3D-сцены. Всегда имеет Transform3D. Сам по себе это пустой
// объект: ничего не рисует, но может быть точкой привязки для детей.
//
// Transform задаётся относительно родителя-Node3D; если родитель не Node3D,
// узел считается корневым в пространстве мира.
class YQLORA_API Node3D : public Node {
public:
    explicit Node3D(std::string name = "Node3D");

    YQLORA_TYPE(Node3D, Node)

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

    // Ставит узел в точку мира: локальное положение пересчитывается от родителя.
    void setWorldPosition(const Vec3& position);

    // Положение, поворот и масштаб относительно родителя, при которых у узла
    // будет мировая матрица world. Углы подбираются ближе к нынешним, наклон
    // (skew) теряется. nullopt, если матрицу не разложить.
    std::optional<TransformValues> localValuesForWorld(const Mat4& world) const;

    // Ставит узлу мировую матрицу, например после манипулятора или переноса
    // с KeepWorld::Yes. Наклон (skew) теряется.
    void setWorldMatrix(const Mat4& world);

    // Направления осей узла в мировом пространстве.
    Vec3 worldForward() const;
    Vec3 worldRight() const;
    Vec3 worldUp() const;

    // Перевод точки из локальных координат узла в мировые и обратно.
    Vec3 toWorld(const Vec3& localPoint) const;
    Vec3 toLocal(const Vec3& worldPoint) const;

protected:
    void onParentChanged() override;
    std::function<void()> captureWorldPosition() override;
    void onEnterScene() override;
    void onExitScene() override;

private:
    friend class Transform3D;
    friend class TransformSystem3D;

    const Node3D* parent3D() const;

    // Свой Transform или родитель изменился: мировая матрица узла и всех
    // Node3D под ним устарела. Если узел уже устаревший, его ветка тоже:
    // свежий узел бывает только под свежими предками. Верхний устаревший
    // узел встаёт в очередь TransformSystem3D сцены.
    void markWorldDirty();
    TransformSystem3D* transformSystem() const;

    Transform3D* m_transform = nullptr; // принадлежит списку компонентов узла

    // Кэш мировой матрицы, пересчитывается при чтении, если устарел.
    mutable Mat4 m_world{1.0f};
    mutable bool m_worldDirty = true;
};

} // namespace yqlora
