#include "yqlora/editor/ui/widgets/PropertyTable.hpp"

#include "yqlora/reflection/TypeRegistry.hpp"

#include <imgui.h>

#include <algorithm>
#include <vector>

namespace yqlora::editor {

namespace {

// Подпись и значение.
constexpr int kColumnCount = 2;
constexpr float kMinLabelWidth = 70.0f;
// Подписи не отнимают у значений больше этой доли ширины.
constexpr float kMaxLabelShare = 0.5f;

// По самой длинной подписи таблицы, с отступами по краям, как у подписей,
// которые можно тянуть.
float labelColumnWidth(const std::vector<const Property*>& properties)
{
    float widest = 0.0f;
    for (const Property* property : properties) {
        widest = std::max(widest, ImGui::CalcTextSize(propertyLabel(property->name()).c_str()).x);
    }
    const float padded = widest + 2.0f * ImGui::GetStyle().FramePadding.x;
    const float limit = std::max(kMinLabelWidth, ImGui::GetContentRegionAvail().x * kMaxLabelShare);

    return std::clamp(padded, kMinLabelWidth, limit);
}

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
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelColumnWidth(properties));
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
