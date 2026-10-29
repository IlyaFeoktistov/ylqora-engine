#include "yqlora/math/Euler.hpp"

#include <cmath>

namespace yqlora::math {

namespace {

constexpr float kFullTurn = 360.0f;
constexpr float kHalfTurn = 180.0f;

// angle + 360 * k, ближайшее к target.
float unwrapNear(float angle, float target)
{
    return angle + kFullTurn * std::round((target - angle) / kFullTurn);
}

Vec3 unwrapNear(const Vec3& angles, const Vec3& target)
{
    return {unwrapNear(angles.x, target.x), unwrapNear(angles.y, target.y), unwrapNear(angles.z, target.z)};
}

float distanceSquared(const Vec3& a, const Vec3& b)
{
    const Vec3 d = a - b;

    return glm::dot(d, d);
}

} // namespace

Vec3 closestEulerDegrees(const Quat& rotation, const Vec3& previousDegrees)
{
    const Vec3 first = glm::degrees(glm::eulerAngles(rotation));
    // Тот же поворот при углах Эйлера XYZ: x + 180, 180 - y, z + 180.
    const Vec3 second(first.x + kHalfTurn, kHalfTurn - first.y, first.z + kHalfTurn);

    const Vec3 a = unwrapNear(first, previousDegrees);
    const Vec3 b = unwrapNear(second, previousDegrees);

    return distanceSquared(a, previousDegrees) <= distanceSquared(b, previousDegrees) ? a : b;
}

} // namespace yqlora::math
