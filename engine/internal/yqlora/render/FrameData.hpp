#pragma once

#include "yqlora/math/DepthRange.hpp"
#include "yqlora/math/Math.hpp"

namespace yqlora {

struct CameraData {
    Mat4 view{1.0f};
    Mat4 projection{1.0f};
    Vec3 position{0.0f};
};

// Всё, что рендереру нужно знать о кадре. Данные собирает buildFrameData,
// рендерер их рисует. Дерево узлов до бэкенда не доходит.
struct FrameData {
    CameraData camera;
    Vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
    // Позже: std::vector<DrawItem> drawItems (меш + материал + transform).
};

} // namespace yqlora
