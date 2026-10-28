#include "panels/InspectorPanel.hpp"

#include "style/Theme.hpp"
#include "widgets/PropertyEditor.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace yqlora::editor {

namespace {

// Подпись и значение.
constexpr int kColumnCount = 2;
constexpr float kLabelShare = 0.3f;
constexpr float kMinLabelWidth = 70.0f;
constexpr float kMaxLabelWidth = 130.0f;

// Возвращает true, если какое-то значение тянут мышью.
template <typename Object>
bool drawPropertyTable(EditorContext& context, const TypeInfo& type, Object& object)
{
    EditorSession& session = context.session;
    const PropertyEditorContext widgets{
        .colors = context.theme.colors(),
        .scene = session.scene(),
        .resources = session.project() ? &session.project()->resources() : nullptr,
    };
    const auto properties = context.registry.properties(type);
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
            session.setProperty(object, *property, result.value);
        }
        dragging = dragging || result.dragging;
    }
    ImGui::EndTable();

    return dragging;
}

void drawAddComponent(EditorSession& session, Node& node)
{
    const std::vector<const TypeInfo*> addable = session.addableComponents(node);
    const bool any = !addable.empty();

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
        for (const TypeInfo* type : addable) {
            if (ImGui::Selectable(type->name().c_str())) {
                session.addComponent(node, *type);
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
    const TypeRegistry& registry = context.registry;
    if (!ImGui::Begin(title().c_str(), visibleFlag())) {
        ImGui::End();

        return;
    }

    Node* node = session.selectedNode();
    const TypeInfo* type = node ? registry.find(node->typeName()) : nullptr;
    if (!node || !type) {
        ImGui::TextDisabled("%s", tr("editor.inspector.nothing").c_str());
        ImGui::End();

        return;
    }

    ImGui::TextDisabled("%s", type->name().c_str());
    ImGui::SetItemTooltip("%s", tr(type->descriptionKey()).c_str());
    bool dragging = drawPropertyTable(context, *type, *node);

    // Удалять компонент можно только после цикла: список меняется.
    Component* toRemove = nullptr;
    for (const auto& component : node->components()) {
        const TypeInfo* componentType = registry.find(component->typeName());
        if (!componentType) {
            continue;
        }
        ImGui::PushID(component.get());
        ImGui::Spacing();

        // Нет крестика у встроенных компонентов и у тех, что нужны другим: их снять нельзя.
        bool keep = true;
        const auto dependents = session.componentDependents(*node, *component);
        const bool removable = node->canRemoveComponent(*component) && dependents.empty();
        const std::string header = componentType->name() + "###component";
        const bool open = ImGui::CollapsingHeader(
            header.c_str(),
            removable ? &keep : nullptr,
            ImGuiTreeNodeFlags_DefaultOpen
        );
        if (!dependents.empty()) {
            std::string names;
            for (const Component* dependent : dependents) {
                names += names.empty() ? "" : ", ";
                names += dependent->typeName();
            }
            ImGui::SetItemTooltip("%s %s", tr("editor.inspector.requiredBy").c_str(), names.c_str());
        }
        if (open) {
            dragging = drawPropertyTable(context, *componentType, *component) || dragging;
        }
        if (!keep) {
            toRemove = component.get();
        }
        ImGui::PopID();
    }
    if (toRemove) {
        session.removeComponent(*node, *toRemove);
    }

    drawAddComponent(session, *node);
    ImGui::End();
    context.mouseWrapping = context.mouseWrapping || dragging;
}

} // namespace yqlora::editor
