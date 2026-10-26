#pragma once

#include "yqlora/math/Math.hpp"

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

// Встроенные меши: "cube" (ребро 1), "sphere" (радиус 0.5), "plane" (1 x 1 на XZ).
// Все с центром в начале координат. nullptr для неизвестного имени.
// Данные живут до конца программы, указатель можно хранить.
const MeshData* findBuiltinMesh(std::string_view name);

// Имена всех встроенных мешей, для выбора в редакторе.
std::span<const std::string_view> builtinMeshNames();

} // namespace yqlora
