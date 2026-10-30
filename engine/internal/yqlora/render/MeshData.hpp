#pragma once

#include "yqlora/math/Math.hpp"

#include <cstdint>
#include <vector>

namespace yqlora {

struct Vertex {
    Vec3 position;
    Vec3 normal;
};

// Геометрия на стороне CPU, откуда бы она ни пришла: встроенная фигура,
// файл модели, код игры. Треугольники перечислены против часовой стрелки,
// если смотреть снаружи: задние грани отсекаются.
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

} // namespace yqlora
