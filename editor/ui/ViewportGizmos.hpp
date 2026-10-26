#pragma once

#include "yqlora/math/Math.hpp"
#include "yqlora/render/FrameData.hpp"

#include <imgui.h>

#include <optional>

namespace yqlora {

class Node;

// Рисунки редактора поверх кадра сцены. origin и size - где картинка сцены
// на экране, в координатах интерфейса.
namespace gizmos {

// Камеры сцены контуром: пирамида обзора и треугольник "верх". У самой камеры
// геометрии нет, без этого её не видно. selected - выбранный узел.
void drawCameras(const Node& root, NodeId selected, const CameraData& view, const ImVec2& origin, const ImVec2& size);

struct AxisGizmoResult {
    // Курсор над кружком оси: клик по сцене в этом кадре не считается выбором объекта.
    bool hovered = false;
    // По оси кликнули: смотреть вдоль этого направления.
    std::optional<Vec3> lookAlong;
};

// Значок осей X, Y, Z в правом верхнем углу: поворачивается вместе с камерой.
AxisGizmoResult drawAxes(const Mat4& view, const ImVec2& origin, const ImVec2& size);

} // namespace gizmos

} // namespace yqlora
