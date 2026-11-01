#include "yqlora/render/MeshData.hpp"

#include <glm/common.hpp>

namespace yqlora {

Bounds computeBounds(const std::vector<Vertex>& vertices)
{
    if (vertices.empty()) {
        return {};
    }
    Bounds bounds{vertices.front().position, vertices.front().position};
    for (const Vertex& vertex : vertices) {
        bounds.min = glm::min(bounds.min, vertex.position);
        bounds.max = glm::max(bounds.max, vertex.position);
    }

    return bounds;
}

void finishSingleMaterial(MeshData& mesh)
{
    SubMesh all;
    all.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
    mesh.submeshes = {all};
    mesh.bounds = computeBounds(mesh.vertices);
}

} // namespace yqlora
