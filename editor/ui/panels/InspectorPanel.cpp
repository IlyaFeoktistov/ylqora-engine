#include "panels/InspectorPanel.hpp"

#include "style/Theme.hpp"
#include "widgets/PropertyEditor.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/Inspection.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace yqlora::editor {

namespace {

// Подпись и значение.
constexpr int kColumnCount = 2;
constexpr float kLabelShare = 0.3f;
constexpr float kMinLabelWidth = 70.0f;
constexpr float kMaxLabelWidth = 130.0f;

// Возвращает true, если какое-то значение тянут мышью.
template <typename Object>
bool drawPropertyTable(
    EditorSession& session,
    const PropertyEditorContext& widgets,
    const TypeInfo& type,
    Object& object
)
{
    const auto properties = session.registry().properties(type);
    if (properties.empty() || !ImGui::BeginTable("properties", kColumnCount)) {
        return false;
    }
    // Подписи узкой колонкой постоянной ширины, остальное место - значениям.
    const float labelWidth = std::clamp(ImGui::GetContentRegionAvail().x * kLabelShare, kMinLabelWidth, kMaxLabelWidth);
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    bool dragging = false;
    for (const Property* property : properties) {
        const PropertyResult result = drawProperty(*property, object, widgets);
        if (result.changed) {
            session.editing().setProperty(object, *property, result.value);
        }
        dragging = dragging || result.dragging;
    }
    ImGui::EndTable();

    return dragging;
}

std::string joinTypeNames(const std::vector<const Component*>& components)
{
    std::string result;
    for (const Component* component : components) {
        result += result.empty() ? "" : ", ";
        result += component->typeName();
    }

    return result;
}

// Возвращает true, если какое-то значение тянут мышью. Компонент, который
// попросили снять, записывает в toRemove: снимать его посреди цикла по списку нельзя.
bool drawComponent(
    EditorSession& session,
    const PropertyEditorContext& widgets,
    const InspectedComponent& inspected,
    Component*& toRemove
)
{
    ImGui::PushID(&inspected.component);
    ImGui::Spacing();

    // Без крестика компонент, который снять нельзя.
    bool keep = true;
    const std::string name = inspected.type ? inspected.type->name() : std::string(inspected.component.typeName());
    const std::string header = name + "###component";
    const bool open = ImGui::CollapsingHeader(
        header.c_str(),
        inspected.removable ? &keep : nullptr,
        ImGuiTreeNodeFlags_DefaultOpen
    );
    if (!inspected.requiredBy.empty()) {
        ImGui::SetItemTooltip(
            "%s %s",
            tr("editor.inspector.requiredBy").c_str(),
            joinTypeNames(inspected.requiredBy).c_str()
        );
    }

    bool dragging = false;
    if (open && inspected.type) {
        dragging = drawPropertyTable(session, widgets, *inspected.type, inspected.component);
    } else if (open) {
        ImGui::TextDisabled("%s", tr("editor.inspector.missing").c_str());
    }
    if (!keep) {
        toRemove = &inspected.component;
    }
    ImGui::PopID();

    return dragging;
}

void drawAddComponent(EditorSession& session, const Inspection& inspection)
{
    const bool any = !inspection.addable.empty();

    ImGui::Spacing();
    ImGui::BeginDisabled(!any);
    if (ImGui::Button(tr("editor.inspector.addComponent").c_str(), ImVec2(-FLT_MIN, 0.0f))) {
        ImGui::OpenPopup("AddComponent");
    }
    ImGui::EndDisabled();
    if (!any) {
        ImGui::SetItemTooltip("%s", tr("editor.inspector.noComponents").c_str());
    }

    if (ImGui::BeginPopup("AddComponent")) {
        for (const TypeInfo* type : inspection.addable) {
            if (ImGui::Selectable(type->name().c_str())) {
                session.editing().addComponent(inspection.node, *type);
            }
            ImGui::SetItemTooltip("%s", tr(type->descriptionKey()).c_str());
        }
        ImGui::EndPopup();
    }
}

} // namespace

void InspectorPanel::draw(EditorContext& context)
{
    EditorSession& session = context.session;
    if (!ImGui::Begin(title().c_str(), visibleFlag())) {
        ImGui::End();

        return;
    }

    const std::optional<Inspection> inspection = inspect(session);
    if (!inspection) {
        ImGui::TextDisabled("%s", tr("editor.inspector.nothing").c_str());
        ImGui::End();

        return;
    }

    const PropertyEditorContext widgets{
        .colors = context.theme.colors(),
        .metrics = context.theme.metrics(),
        .scene = session.scene(),
        .resources = session.project() ? &session.project()->resources() : nullptr,
    };
    ImGui::TextDisabled("%s", inspection->type.name().c_str());
    ImGui::SetItemTooltip("%s", tr(inspection->type.descriptionKey()).c_str());
    bool dragging = drawPropertyTable(session, widgets, inspection->type, inspection->node);

    Component* toRemove = nullptr;
    for (const InspectedComponent& inspected : inspection->components) {
        dragging = drawComponent(session, widgets, inspected, toRemove) || dragging;
    }
    if (toRemove) {
        session.editing().removeComponent(inspection->node, *toRemove);
    }

    drawAddComponent(session, *inspection);
    ImGui::End();
    context.mouseWrapping = context.mouseWrapping || dragging;
}

} // namespace yqlora::editor
