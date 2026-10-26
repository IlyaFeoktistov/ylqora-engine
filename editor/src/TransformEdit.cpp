#include "yqlora/editor/TransformEdit.hpp"

#include "yqlora/scene/Node3D.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#include <cmath>

namespace yqlora {

namespace {

// angle + 360 * k, ближайшее к target.
float unwrapNear(float angle, float target)
{
    return angle + 360.0f * std::round((target - angle) / 360.0f);
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
    const Vec3 second(first.x + 180.0f, 180.0f - first.y, first.z + 180.0f);

    const Vec3 a = unwrapNear(first, previousDegrees);
    const Vec3 b = unwrapNear(second, previousDegrees);

    return distanceSquared(a, previousDegrees) <= distanceSquared(b, previousDegrees) ? a : b;
}

void setWorldMatrix(Node3D& node, const Mat4& world)
{
    const auto* parent = dynamic_cast<const Node3D*>(node.parent());
    const Mat4 local = parent ? glm::inverse(parent->worldMatrix()) * world : world;

    Vec3 scale;
    Quat rotation;
    Vec3 translation;
    Vec3 skew;
    Vec4 perspective;
    if (!glm::decompose(local, scale, rotation, translation, skew, perspective)) {
        return;
    }

    Transform3D& transform = node.transform();
    transform.setPosition(translation);
    transform.setRotationDegrees(closestEulerDegrees(rotation, transform.rotationDegrees()));
    transform.setScale(scale);
}

} // namespace yqlora
