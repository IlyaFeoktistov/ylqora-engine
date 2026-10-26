#pragma once

#include "Gizmo.hpp"

namespace yqlora {

// Значок осей X, Y, Z в правом верхнем углу: поворачивается вместе с камерой.
// Клик по кружку оси поворачивает камеру редактора вдоль этой оси.
class AxisGizmo final : public Gizmo {
public:
    bool draw(GizmoContext& context) override;
};

} // namespace yqlora
