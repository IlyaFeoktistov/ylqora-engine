#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace yqlora {

// Диапазон глубины в clip space различается между API:
// OpenGL — [-1, 1], Vulkan/Direct3D — [0, 1]. Проекцию строим под рендерер.
enum class DepthRange {
    MinusOneToOne,
    ZeroToOne,
};

struct CameraData {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::vec3 position{0.0f};
};

// Всё, что рендереру нужно знать о кадре. Сцена собирает эти данные,
// рендерер их рисует — дерево узлов до бэкенда не доходит.
struct FrameData {
    CameraData camera;
    glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
    // Позже: std::vector<DrawItem> drawItems (меш + материал + transform).
};

} // namespace yqlora
