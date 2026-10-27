#include "SelectionGizmo.hpp"

#include "Theme.hpp"

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/Picking.hpp"

namespace yqlora {

void SelectionGizmo::prepareFrame(FrameData& frame, const EditorSession& session)
{
    frame.editor.selected = session.selection().selected();
    frame.editor.outlineColor = theme::colors().selectionOutline;
}

bool SelectionGizmo::draw(GizmoContext& context)
{
    if (!context.mouseFree || context.camera.flying() || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return false;
    }
    const ImVec2 mouse = ImGui::GetMousePos();
    const Ray ray = context.camera.screenRay(
        {mouse.x - context.origin.x, mouse.y - context.origin.y},
        {context.size.x, context.size.y}
    );
    context.session.selection().select(pickNode(context.session.scene(), ray));

    return false;
}

} // namespace yqlora
