#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"

namespace yqlora::math {

// Углы Эйлера в градусах для поворота rotation, ближайшие к previousDegrees.
// Один поворот можно записать разными углами: (0, 100, 0) и (180, 80, 180),
// а ещё с точностью до 360 градусов. Без подбора цифры в инспекторе скакали бы,
// пока объект крутят манипулятором.
YQLORA_API Vec3 closestEulerDegrees(const Quat& rotation, const Vec3& previousDegrees);

} // namespace yqlora::math
