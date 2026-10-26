#include "Gizmo.hpp"

namespace yqlora {

void GizmoLayer::add(std::unique_ptr<Gizmo> gizmo)
{
    m_gizmos.push_back(std::move(gizmo));
}

void GizmoLayer::prepareFrame(FrameData& frame, const EditorSession& session) const
{
    if (!m_enabled) {
        return;
    }
    for (const auto& gizmo : m_gizmos) {
        gizmo->prepareFrame(frame, session);
    }
}

bool GizmoLayer::draw(GizmoContext& context)
{
    if (!m_enabled) {
        return false;
    }
    bool mouseTaken = false;
    for (const auto& gizmo : m_gizmos) {
        if (gizmo->draw(context)) {
            mouseTaken = true;
            context.mouseFree = false;
        }
    }

    return mouseTaken;
}

} // namespace yqlora
