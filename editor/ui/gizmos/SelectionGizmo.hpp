#pragma once

#include "Gizmo.hpp"

namespace yqlora::editor {

// Выделение в окне сцены: контур вокруг выбранного узла и выбор узла кликом.
// Клик в пустоту снимает выделение. Стоит последним: клик, который занял
// другой гизмо (манипулятор, оси), выбором не считается.
class SelectionGizmo final : public Gizmo {
public:
    void prepareFrame(FrameData& frame, const GizmoContext& context) override;
    bool draw(GizmoContext& context) override;
};

} // namespace yqlora::editor
