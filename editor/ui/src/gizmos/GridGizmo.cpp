#include "yqlora/editor/ui/gizmos/GridGizmo.hpp"

namespace yqlora::editor {

void GridGizmo::prepareFrame(FrameData& frame, const GizmoContext& /*context*/)
{
    frame.editor.grid = true;
}

} // namespace yqlora::editor
