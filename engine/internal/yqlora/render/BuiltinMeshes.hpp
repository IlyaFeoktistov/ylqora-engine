#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/render/MeshData.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <span>
#include <string_view>

namespace yqlora {

// Меш, встроенный в движок: стандартный выбор для MeshRenderer. ID
// постоянные, см. BuiltinResources.hpp; сами данные держит ResourceCache.
struct BuiltinMesh {
    ResourceId id;
    // Английское имя: "cube", "sphere", "plane".
    std::string_view name;
    MeshData (*make)();
};

// Все встроенные меши, для кэша ресурсов и для выбора в редакторе.
YQLORA_API std::span<const BuiltinMesh> builtinMeshes();

} // namespace yqlora
