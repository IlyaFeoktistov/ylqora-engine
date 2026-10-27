#pragma once

#include "Gizmo.hpp"

namespace yqlora::editor {

// Бесконечная сетка на плоскости Y = 0. Рисует её рендерер, гизмо только включает.
class GridGizmo final : public Gizmo {
public:
    void prepareFrame(FrameData& frame, const EditorSession& session) override;
};

} // namespace yqlora::editor
