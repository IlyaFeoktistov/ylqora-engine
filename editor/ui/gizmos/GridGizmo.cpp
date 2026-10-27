#include "GridGizmo.hpp"

namespace yqlora::editor {

void GridGizmo::prepareFrame(FrameData& frame, const EditorSession& /*session*/)
{
    frame.editor.grid = true;
}

} // namespace yqlora::editor
