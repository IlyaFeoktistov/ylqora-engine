#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/render/MeshData.hpp"

namespace yqlora {

// Простые фигуры с центром в начале координат.
YQLORA_API MeshData makeCube();   // ребро 1
YQLORA_API MeshData makeSphere(); // радиус 0.5
YQLORA_API MeshData makePlane();  // 1 x 1 на плоскости XZ, нормаль вверх

} // namespace yqlora
