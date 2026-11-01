#include "yqlora/editor/Picking.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/render/Camera3D.hpp"
#include "yqlora/render/MeshData.hpp"
#include "yqlora/render/MeshRenderer.hpp"
#include "yqlora/resource/ResourceCache.hpp"
#include "yqlora/scene/Scene.hpp"

#include <glm/geometric.hpp>

#include <limits>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace yqlora::editor {

namespace {

constexpr std::size_t kTriangleVertices = 3;

// Пересечение луча с треугольником (алгоритм Мёллера - Трумбора).
// Возвращает t точки пересечения; обе стороны треугольника считаются.
std::optional<float> intersect(const Ray& ray, const Vec3& a, const Vec3& b, const Vec3& c)
{
    constexpr float kEpsilon = 1e-7f;
    const Vec3 edge1 = b - a;
    const Vec3 edge2 = c - a;
    const Vec3 p = glm::cross(ray.direction, edge2);
    const float determinant = glm::dot(edge1, p);
    if (std::abs(determinant) < kEpsilon) {
        return std::nullopt; // луч параллелен треугольнику
    }
    const float inverse = 1.0f / determinant;
    const Vec3 s = ray.origin - a;
    const float u = glm::dot(s, p) * inverse;
    if (u < 0.0f || u > 1.0f) {
        return std::nullopt;
    }
    const Vec3 q = glm::cross(s, edge1);
    const float v = glm::dot(ray.direction, q) * inverse;
    if (v < 0.0f || u + v > 1.0f) {
        return std::nullopt;
    }
    const float t = glm::dot(edge2, q) * inverse;

    return t > kEpsilon ? std::optional<float>(t) : std::nullopt;
}

// Ближайшее пересечение луча со сферой; t в единицах направления луча, как у треугольников.
std::optional<float> intersectSphere(const Ray& ray, const Vec3& center, float radius)
{
    const Vec3 offset = ray.origin - center;
    const float a = glm::dot(ray.direction, ray.direction);
    const float b = glm::dot(offset, ray.direction);
    const float c = glm::dot(offset, offset) - radius * radius;
    const float discriminant = b * b - a * c;
    if (discriminant < 0.0f || a == 0.0f) {
        return std::nullopt;
    }
    const float root = std::sqrt(discriminant);
    const float nearT = (-b - root) / a;
    const float farT = (-b + root) / a;
    if (nearT > 0.0f) {
        return nearT;
    }

    return farT > 0.0f ? std::optional<float>(farT) : std::nullopt;
}

// Задевает ли луч границы (метод плит). Луч, параллельный плите, задевает,
// только если начинается между её плоскостями.
bool hitsBounds(const Ray& ray, const Bounds& bounds)
{
    float nearT = 0.0f;
    float farT = std::numeric_limits<float>::max();
    for (int axis = 0; axis < Vec3::length(); ++axis) {
        if (ray.direction[axis] == 0.0f) {
            if (ray.origin[axis] < bounds.min[axis] || ray.origin[axis] > bounds.max[axis]) {
                return false;
            }
            continue;
        }
        float t1 = (bounds.min[axis] - ray.origin[axis]) / ray.direction[axis];
        float t2 = (bounds.max[axis] - ray.origin[axis]) / ray.direction[axis];
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        nearT = std::max(nearT, t1);
        farT = std::min(farT, t2);
        if (nearT > farT) {
            return false;
        }
    }

    return true;
}

struct Hit {
    NodeId node = 0;
    float t = 0.0f;
};

void keepNearest(std::optional<Hit>& best, NodeId node, std::optional<float> t)
{
    if (t && (!best || *t < best->t)) {
        best = Hit{node, *t};
    }
}

// Камеры - узлы, а не компоненты, поэтому их ищем по дереву.
void pickCameras(const Node& node, const Ray& worldRay, std::optional<Hit>& best)
{
    // Неактивную ветку не видно, и выбрать её можно только в дереве.
    if (!node.activeInHierarchy()) {
        return;
    }
    if (const auto* camera = dynamic_cast<const Camera3D*>(&node)) {
        keepNearest(best, camera->id(), intersectSphere(worldRay, camera->worldPosition(), kCameraPickRadius));
    }
    for (const auto& child : node.children()) {
        pickCameras(*child, worldRay, best);
    }
}

void pickMesh(const Node3D& node, const MeshData& mesh, const Ray& worldRay, std::optional<Hit>& best)
{
    // Луч переводим в координаты меша, а не меш в мир. Направление
    // не нормализуем: тогда t в локальных координатах тот же, что в мире,
    // и попадания в разные объекты можно сравнивать.
    const Mat4 toLocal = glm::inverse(node.worldMatrix());
    const Ray localRay{Vec3(toLocal * Vec4(worldRay.origin, 1.0f)), Vec3(toLocal * Vec4(worldRay.direction, 0.0f))};
    if (!hitsBounds(localRay, mesh.bounds)) {
        return;
    }
    for (std::size_t i = 0; i + kTriangleVertices <= mesh.indices.size(); i += kTriangleVertices) {
        const auto t = intersect(
            localRay,
            mesh.vertices[mesh.indices[i]].position,
            mesh.vertices[mesh.indices[i + 1]].position,
            mesh.vertices[mesh.indices[i + 2]].position
        );
        keepNearest(best, node.id(), t);
    }
}

} // namespace

NodeId pickNode(const Scene& scene, const Ray& ray)
{
    if (!scene.root()) {
        return 0;
    }
    std::optional<Hit> best;
    pickCameras(*scene.root(), ray, best);
    ResourceCache& resources = scene.services().resources();
    for (const MeshRenderer* renderer : scene.components<MeshRenderer>()) {
        if (!renderer->activeAndEnabled()) {
            continue;
        }
        const auto* node = dynamic_cast<const Node3D*>(renderer->owner());
        if (const MeshData* mesh = resources.find<MeshData>(renderer->mesh().id); node && mesh) {
            pickMesh(*node, *mesh, ray, best);
        }
    }

    return best ? best->node : 0;
}

} // namespace yqlora::editor
