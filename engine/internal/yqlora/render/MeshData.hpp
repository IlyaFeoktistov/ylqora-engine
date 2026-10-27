#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace yqlora {

struct Vertex {
    Vec3 position;
    Vec3 normal;
};

// Геометрия на стороне CPU. Треугольники перечислены против часовой стрелки,
// если смотреть снаружи: задние грани отсекаются.
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

// Встроенный меш с центром в начале координат (см. BuiltinResources.hpp).
// nullptr, если это не встроенный меш. Данные живут до конца программы.
YQLORA_API const MeshData* findBuiltinMesh(ResourceId id);

struct BuiltinMesh {
    ResourceId id;
    // Английское имя: "cube", "sphere", "plane".
    std::string_view name;
};

// Все встроенные меши, для выбора в редакторе.
YQLORA_API std::span<const BuiltinMesh> builtinMeshes();

} // namespace yqlora
