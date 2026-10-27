#pragma once

#include "yqlora/math/Math.hpp"

#include <optional>

namespace yqlora {

class Node3D;
class Transform3D;

} // namespace yqlora

namespace yqlora::editor {

// Положение, поворот и масштаб узла относительно родителя, как их видно в инспекторе.
struct TransformValues {
    Vec3 position{0.0f};
    Vec3 rotationDegrees{0.0f};
    Vec3 scale{1.0f};

    bool operator==(const TransformValues&) const = default;
};

TransformValues transformValues(const Transform3D& transform);
void setTransformValues(Transform3D& transform, const TransformValues& values);

// Углы Эйлера в градусах для поворота rotation, ближайшие к previousDegrees.
// Один поворот можно записать разными углами: (0, 100, 0) и (180, 80, 180),
// а ещё с точностью до 360 градусов. Без подбора цифры в инспекторе скакали бы,
// пока объект крутят манипулятором.
Vec3 closestEulerDegrees(const Quat& rotation, const Vec3& previousDegrees);

// Положение, поворот и масштаб относительно родителя, при которых у узла будет
// мировая матрица world. Углы подбираются ближе к нынешним. Наклон (skew) теряется.
// nullopt, если матрицу не разложить.
std::optional<TransformValues> localValuesForWorld(const Node3D& node, const Mat4& world);

// Ставит узлу мировую матрицу, например после манипулятора: переводит её
// в положение, поворот и масштаб относительно родителя. Наклон (skew) теряется.
void setWorldMatrix(Node3D& node, const Mat4& world);

} // namespace yqlora::editor
