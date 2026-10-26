#include "GridGizmo.hpp"

namespace yqlora {

void GridGizmo::prepareFrame(FrameData& frame, const EditorSession& /*session*/)
{
    frame.editor.grid = true;
}

} // namespace yqlora
