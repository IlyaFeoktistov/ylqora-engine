#pragma once

#include "yqlora/editor/ui/gizmos/Gizmo.hpp"

namespace yqlora::editor {

// Камеры сцены контуром: пирамида обзора и треугольник "верх". У самой камеры
// геометрии нет, без этого её не видно. Выбранная камера - цветом выделения.
class CameraGizmo final : public Gizmo {
public:
    bool draw(GizmoContext& context) override;
};

} // namespace yqlora::editor
