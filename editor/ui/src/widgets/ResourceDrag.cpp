#include "yqlora/editor/ui/widgets/ResourceDrag.hpp"

#include <imgui.h>

namespace yqlora::editor {

namespace {

// Тип данных перетаскивания в ImGui, не длиннее 32 символов.
constexpr const char* kResourcePayload = "yqlora.resource";

} // namespace

void resourceDragSource(ResourceId id, const std::string& label)
{
    if (id == 0 || !ImGui::BeginDragDropSource()) {
        return;
    }
    ImGui::SetDragDropPayload(kResourcePayload, &id, sizeof(id));
    ImGui::TextUnformatted(label.c_str());
    ImGui::EndDragDropSource();
}

std::optional<ResourceId> acceptResourceDrop()
{
    if (!ImGui::BeginDragDropTarget()) {
        return std::nullopt;
    }
    std::optional<ResourceId> dropped;
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kResourcePayload)) {
        dropped = *static_cast<const ResourceId*>(payload->Data);
    }
    ImGui::EndDragDropTarget();

    return dropped;
}

} // namespace yqlora::editor
