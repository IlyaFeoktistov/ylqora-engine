#include "yqlora/editor/Snapping.hpp"

#include "yqlora/editor/Picking.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/quaternion.hpp>

namespace yqlora::editor {

Vec3 snapToGrid(const Vec3& point, float step)
{
    if (step <= 0.0f) {
        return point;
    }

    return glm::round(point / step) * step;
}

Vec3 dropPoint(const Scene& scene, const Ray& ray, const SnapSettings& snap)
{
    const auto hit = raycastSurfaces(scene, ray);
    const Vec3 point = hit ? hit->point : placementPoint(ray);

    return snap.grid ? snapToGrid(point, snap.moveStep) : point;
}

Quat alignUpTo(const Quat& rotation, const Vec3& normal)
{
    const float length = glm::length(normal);
    if (length == 0.0f) {
        return rotation;
    }
    const Vec3 up = rotation * Vec3(0.0f, 1.0f, 0.0f);

    return glm::normalize(glm::rotation(up, normal / length) * rotation);
}

} // namespace yqlora::editor
