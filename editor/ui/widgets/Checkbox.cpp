#include "widgets/Checkbox.hpp"

#include <imgui.h>

#include <algorithm>

namespace yqlora::editor {

bool checkbox(const char* label, bool* value, float padding)
{
    const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0f, framePadding.y - padding));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding.x, padding));
    const bool changed = ImGui::Checkbox(label, value);
    ImGui::PopStyleVar();

    return changed;
}

} // namespace yqlora::editor
