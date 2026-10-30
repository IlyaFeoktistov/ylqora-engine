#include "yqlora/render/BuiltinMeshes.hpp"

#include "yqlora/render/PrimitiveMeshes.hpp"
#include "yqlora/resource/BuiltinResources.hpp"

#include <algorithm>

namespace yqlora {

std::span<const BuiltinMesh> builtinMeshes()
{
    static const MeshData cube = makeCube();
    static const MeshData sphere = makeSphere();
    static const MeshData plane = makePlane();
    static const BuiltinMesh meshes[] = {
        {builtin::kCubeMesh, "cube", &cube},
        {builtin::kSphereMesh, "sphere", &sphere},
        {builtin::kPlaneMesh, "plane", &plane},
    };

    return meshes;
}

const MeshData* findBuiltinMesh(ResourceId id)
{
    const auto meshes = builtinMeshes();
    const auto it = std::ranges::find(meshes, id, &BuiltinMesh::id);

    return it != meshes.end() ? it->data : nullptr;
}

} // namespace yqlora
