#pragma once

#include "yqlora/math/Math.hpp"

namespace yqlora {

class Node3D;

// Углы Эйлера в градусах для поворота rotation, ближайшие к previousDegrees.
// Один поворот можно записать разными углами: (0, 100, 0) и (180, 80, 180),
// а ещё с точностью до 360 градусов. Без подбора цифры в инспекторе скакали бы,
// пока объект крутят манипулятором.
Vec3 closestEulerDegrees(const Quat& rotation, const Vec3& previousDegrees);

// Ставит узлу мировую матрицу, например после манипулятора: переводит её
// в положение, поворот и масштаб относительно родителя. Наклон (skew) теряется.
void setWorldMatrix(Node3D& node, const Mat4& world);

} // namespace yqlora
