#include "SceneTreePanel.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/scene/Scene.hpp"

#include <imgui.h>

#include <cstdint>
#include <string>

namespace yqlora {

namespace {

bool contains(const Node& node, NodeId id)
{
    if (node.id() == id) {
        return true;
    }
    for (const auto& child : node.children()) {
        if (contains(*child, id)) {
            return true;
        }
    }

    return false;
}

} // namespace

void SceneTreePanel::draw(EditorSession& session, const TypeRegistry& registry)
{
    const Scene& scene = session.scene();
    Selection& selection = session.selection();
    m_reveal = selection.selected() != m_lastSelected;

    const std::string title = tr("editor.window.scene") + "###Scene";
    if (ImGui::Begin(title.c_str())) {
        if (ImGui::Button("+")) {
            m_addNode.open(selection.selected());
        }
        ImGui::SetItemTooltip("%s", tr("editor.tree.addNode").c_str());
        ImGui::Separator();

        if (const Node* root = scene.root()) {
            drawNode(*root, selection);
        } else {
            ImGui::TextDisabled("%s", tr("editor.scene.empty").c_str());
        }

        // Клик по пустому месту панели снимает выбор, правый клик - меню.
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
            selection.clear();
        }
        if (ImGui::BeginPopupContextWindow("TreeMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem(tr("editor.tree.addNode").c_str())) {
                m_addNode.open(0);
            }
            ImGui::EndPopup();
        }
        if (ImGui::IsWindowFocused() && selection.selected() != 0 && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            m_deleteRequest = selection.selected();
        }

        m_addNode.draw(session, registry);
    }
    ImGui::End();

    if (m_deleteRequest != 0) {
        session.deleteNode(m_deleteRequest);
        m_deleteRequest = 0;
    }
    m_lastSelected = selection.selected();
}

void SceneTreePanel::drawNode(const Node& node, Selection& selection)
{
    const bool selected = selection.isSelected(node.id());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
                               | ImGuiTreeNodeFlags_DefaultOpen;
    if (node.children().empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    // Выбор пришёл из окна сцены: раскрываем путь до выбранного узла.
    if (m_reveal && !selected && contains(node, selection.selected())) {
        ImGui::SetNextItemOpen(true);
    }

    const auto id = reinterpret_cast<void*>(static_cast<std::uintptr_t>(node.id()));
    const bool open = ImGui::TreeNodeEx(id, flags, "%s", node.name().c_str());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
        selection.select(node.id());
        m_reveal = false;
    }
    if (ImGui::BeginPopupContextItem()) {
        selection.select(node.id());
        if (ImGui::MenuItem(tr("editor.tree.addChild").c_str())) {
            m_addNode.open(node.id());
        }
        if (ImGui::MenuItem(tr("editor.tree.delete").c_str(), "Delete")) {
            m_deleteRequest = node.id();
        }
        ImGui::EndPopup();
    }
    if (selected && m_reveal) {
        ImGui::SetScrollHereY();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", std::string(node.typeName()).c_str());

    if (open) {
        for (const auto& child : node.children()) {
            drawNode(*child, selection);
        }
        ImGui::TreePop();
    }
}

} // namespace yqlora
