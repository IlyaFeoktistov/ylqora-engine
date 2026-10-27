#pragma once

#include "yqlora/math/DepthRange.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node.hpp"

#include <vector>

namespace yqlora {

struct MeshData;

struct CameraData {
    Mat4 view{1.0f};
    Mat4 projection{1.0f};
    Vec3 position{0.0f};
};

// Один меш в кадре.
struct DrawItem {
    const MeshData* mesh = nullptr;
    Mat4 world{1.0f};
    Vec4 color{1.0f};
    NodeId node = 0; // чей это меш: по нему рендерер узнаёт выбранный объект
};

// Вспомогательная графика редактора, которой нет в игре.
struct EditorOverlay {
    bool grid = false;
    // Выбранные узлы обводятся контуром.
    std::vector<NodeId> selected;
    Vec4 outlineColor{1.0f, 0.6f, 0.1f, 1.0f};
};

// Всё, что рендереру нужно знать о кадре. Данные собирает buildFrameData,
// рендерер их рисует. Дерево узлов до бэкенда не доходит.
struct FrameData {
    CameraData camera;
    Vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<DrawItem> drawItems;
    EditorOverlay editor;
};

} // namespace yqlora
