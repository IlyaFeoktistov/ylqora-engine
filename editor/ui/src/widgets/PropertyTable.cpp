#include "yqlora/editor/ui/widgets/PropertyTable.hpp"

#include "yqlora/reflection/TypeRegistry.hpp"

#include <imgui.h>

#include <algorithm>

namespace yqlora::editor {

namespace {

// Подпись и значение.
constexpr int kColumnCount = 2;
constexpr float kLabelShare = 0.3f;
constexpr float kMinLabelWidth = 70.0f;
constexpr float kMaxLabelWidth = 130.0f;

template <typename Object>
bool drawTable(
    const TypeRegistry& registry,
    const PropertyEditorContext& widgets,
    const TypeInfo& type,
    Object& object,
    const PropertyChanged& changed
)
{
    const auto properties = registry.properties(type);
    if (properties.empty() || !ImGui::BeginTable("properties", kColumnCount)) {
        return false;
    }
    const float labelWidth = std::clamp(ImGui::GetContentRegionAvail().x * kLabelShare, kMinLabelWidth, kMaxLabelWidth);
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    bool dragging = false;
    for (const Property* property : properties) {
        const PropertyResult result = drawProperty(*property, object, widgets);
        if (result.changed) {
            changed(*property, result.value);
        }
        dragging = dragging || result.dragging;
    }
    ImGui::EndTable();

    return dragging;
}

} // namespace

bool drawPropertyTable(
    const TypeRegistry& registry,
    const PropertyEditorContext& widgets,
    const TypeInfo& type,
    Node& node,
    const PropertyChanged& changed
)
{
    return drawTable(registry, widgets, type, node, changed);
}

bool drawPropertyTable(
    const TypeRegistry& registry,
    const PropertyEditorContext& widgets,
    const TypeInfo& type,
    Component& component,
    const PropertyChanged& changed
)
{
    return drawTable(registry, widgets, type, component, changed);
}

bool drawPropertyTable(
    const TypeRegistry& registry,
    const PropertyEditorContext& widgets,
    const TypeInfo& type,
    SettingsSection& section,
    const PropertyChanged& changed
)
{
    return drawTable(registry, widgets, type, section, changed);
}

} // namespace yqlora::editor
