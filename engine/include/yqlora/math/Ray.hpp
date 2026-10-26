#pragma once

#include "yqlora/math/Math.hpp"

namespace yqlora {

// Луч: точки origin + direction * t при t >= 0. direction не обязан быть единичным.
struct Ray {
    Vec3 origin{0.0f};
    Vec3 direction{0.0f, 0.0f, -1.0f};

    Vec3 at(float t) const
    {
        return origin + direction * t;
    }
};

} // namespace yqlora
