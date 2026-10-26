#pragma once

namespace yqlora {

// Диапазон глубины в clip space различается между API:
// OpenGL: [-1, 1], Vulkan/Direct3D: [0, 1]. Проекцию строим под рендерер.
enum class DepthRange {
    MinusOneToOne,
    ZeroToOne,
};

} // namespace yqlora
