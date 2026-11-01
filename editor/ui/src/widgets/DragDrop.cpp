#include "yqlora/editor/ui/widgets/DragDrop.hpp"

#include <imgui.h>

#include <cstdint>

namespace yqlora::editor {

namespace {

// Типы данных перетаскивания в ImGui, не длиннее 32 символов.
constexpr const char* kResourcePayload = "yqlora.resource";
constexpr const char* kNodePayload = "yqlora.node";

void dragSource(const char* type, std::uint64_t id, const std::string& label)
{
    if (id == 0 || !ImGui::BeginDragDropSource()) {
        return;
    }
    ImGui::SetDragDropPayload(type, &id, sizeof(id));
    ImGui::TextUnformatted(label.c_str());
    ImGui::EndDragDropSource();
}

std::optional<std::uint64_t> acceptDrop(const char* type)
{
    if (!ImGui::BeginDragDropTarget()) {
        return std::nullopt;
    }
    std::optional<std::uint64_t> dropped;
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(type)) {
        dropped = *static_cast<const std::uint64_t*>(payload->Data);
    }
    ImGui::EndDragDropTarget();

    return dropped;
}

} // namespace

void resourceDragSource(ResourceId id, const std::string& label)
{
    dragSource(kResourcePayload, id, label);
}

std::optional<ResourceId> acceptResourceDrop()
{
    return acceptDrop(kResourcePayload);
}

void nodeDragSource(NodeId id, const std::string& label)
{
    dragSource(kNodePayload, id, label);
}

std::optional<NodeId> acceptNodeDrop()
{
    return acceptDrop(kNodePayload);
}

} // namespace yqlora::editor
