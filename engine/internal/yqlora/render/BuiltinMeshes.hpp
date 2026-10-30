#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/render/MeshData.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <span>
#include <string_view>

namespace yqlora {

// Меш, встроенный в движок: стандартный выбор для MeshRenderer, пока у
// проекта нет своих мешей. ID постоянные, см. BuiltinResources.hpp.
struct BuiltinMesh {
    ResourceId id;
    // Английское имя: "cube", "sphere", "plane".
    std::string_view name;
    // Живёт до конца программы.
    const MeshData* data;
};

// Все встроенные меши, для выбора в редакторе.
YQLORA_API std::span<const BuiltinMesh> builtinMeshes();

// nullptr, если это не встроенный меш.
YQLORA_API const MeshData* findBuiltinMesh(ResourceId id);

} // namespace yqlora
