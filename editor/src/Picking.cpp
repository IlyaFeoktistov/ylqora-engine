#include "yqlora/editor/Picking.hpp"

#include "yqlora/render/MeshData.hpp"
#include "yqlora/scene/MeshInstance3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <optional>

namespace yqlora {

namespace {

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

struct Hit {
    NodeId node = 0;
    float t = 0.0f;
};

void pick(const Node& node, const Ray& worldRay, std::optional<Hit>& best)
{
    if (const auto* instance = dynamic_cast<const MeshInstance3D*>(&node)) {
        if (const MeshData* mesh = findBuiltinMesh(instance->mesh())) {
            // Луч переводим в координаты меша, а не меш в мир. Направление
            // не нормализуем: тогда t в локальных координатах тот же, что в мире,
            // и попадания в разные объекты можно сравнивать.
            const Mat4 toLocal = glm::inverse(instance->worldMatrix());
            const Ray localRay{Vec3(toLocal * Vec4(worldRay.origin, 1.0f)), Vec3(toLocal * Vec4(worldRay.direction, 0.0f))};

            for (std::size_t i = 0; i + 2 < mesh->indices.size(); i += 3) {
                const auto t = intersect(localRay, mesh->vertices[mesh->indices[i]].position,
                                         mesh->vertices[mesh->indices[i + 1]].position,
                                         mesh->vertices[mesh->indices[i + 2]].position);
                if (t && (!best || *t < best->t)) {
                    best = Hit{instance->id(), *t};
                }
            }
        }
    }
    for (const auto& child : node.children()) {
        pick(*child, worldRay, best);
    }
}

} // namespace

NodeId pickNode(const Scene& scene, const Ray& ray)
{
    if (!scene.root()) {
        return 0;
    }
    std::optional<Hit> best;
    pick(*scene.root(), ray, best);

    return best ? best->node : 0;
}

} // namespace yqlora
