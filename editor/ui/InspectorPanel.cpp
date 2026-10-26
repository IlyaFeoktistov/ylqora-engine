#include "InspectorPanel.hpp"

#include "PropertyEditor.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

#include <imgui.h>

#include <cstdint>
#include <string>

namespace yqlora {

namespace {

template <typename Object>
void drawPropertyTable(EditorSession& session, const TypeRegistry& registry, const TypeInfo& type, Object& object)
{
    const auto properties = registry.properties(type);
    if (properties.empty()) {
        return;
    }
    if (!ImGui::BeginTable("properties", 2, ImGuiTableFlags_SizingStretchProp)) {
        return;
    }
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch, 0.4f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 0.6f);
    for (const Property* property : properties) {
        if (drawProperty(*property, object)) {
            session.markModified();
        }
    }
    ImGui::EndTable();
}

void drawAddComponent(EditorSession& session, const TypeRegistry& registry, Node& node)
{
    bool any = false;
    for (const auto& type : registry.types()) {
        any = any || (type->kind() == TypeKind::Component && type->creatable());
    }

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
        for (const auto& type : registry.types()) {
            if (type->kind() != TypeKind::Component || !type->creatable()) {
                continue;
            }
            if (ImGui::Selectable(type->name().c_str())) {
                session.addComponent(node, *type);
            }
            ImGui::SetItemTooltip("%s", tr(type->descriptionKey()).c_str());
        }
        ImGui::EndPopup();
    }
}

} // namespace

void InspectorPanel::draw(EditorSession& session, const TypeRegistry& registry)
{
    const std::string title = tr("editor.window.inspector") + "###Inspector";
    if (!ImGui::Begin(title.c_str())) {
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
    drawPropertyTable(session, registry, *type, *node);

    // Удалять компонент можно только после цикла: список меняется.
    Component* toRemove = nullptr;
    for (const auto& component : node->components()) {
        const TypeInfo* componentType = registry.find(component->typeName());
        if (!componentType) {
            continue;
        }
        ImGui::PushID(component.get());
        ImGui::Spacing();

        // У встроенных компонентов нет крестика: их снять нельзя.
        bool keep = true;
        const bool removable = node->canRemoveComponent(*component);
        const std::string header = componentType->name() + "###component";
        if (ImGui::CollapsingHeader(header.c_str(), removable ? &keep : nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
            drawPropertyTable(session, registry, *componentType, *component);
        }
        if (!keep) {
            toRemove = component.get();
        }
        ImGui::PopID();
    }
    if (toRemove) {
        session.removeComponent(*node, *toRemove);
    }

    drawAddComponent(session, registry, *node);
    ImGui::End();
}

} // namespace yqlora
