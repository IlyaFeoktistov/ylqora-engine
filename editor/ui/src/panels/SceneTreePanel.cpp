#include "yqlora/editor/ui/panels/SceneTreePanel.hpp"

#include "yqlora/editor/ui/style/Icons.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/scene/NodeTree.hpp"
#include "yqlora/scene/Scene.hpp"

#include <imgui.h>

#include <cstdint>
#include <string>
#include <vector>

namespace yqlora::editor {

namespace {

} // namespace

void SceneTreePanel::draw(EditorContext& context)
{
    EditorSession& session = context.session;
    const Scene& scene = session.scene();
    Selection& selection = session.selection();
    m_reveal = selection.selected() != m_lastSelected;

    if (ImGui::Begin(title().c_str(), visibleFlag())) {
        if (ImGui::Button(icons::kAdd)) {
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
        if (ImGui::BeginPopupContextWindow(
                "TreeMenu",
                ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems
            )) {
            if (ImGui::MenuItem(tr("editor.tree.addNode").c_str())) {
                m_addNode.open(0);
            }
            ImGui::EndPopup();
        }
        if (ImGui::IsWindowFocused() && !selection.empty() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            m_deleteRequested = true;
        }

        m_addNode.draw(session, context.registry);
    }
    ImGui::End();

    if (m_deleteRequested) {
        // Копия: удаление меняет сам выбор.
        const std::vector<NodeId> nodes(selection.nodes().begin(), selection.nodes().end());
        session.editing().deleteNodes(nodes);
        m_deleteRequested = false;
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
    if (m_reveal && !selected && findNode(node, selection.selected())) {
        ImGui::SetNextItemOpen(true);
    }

    const auto id = reinterpret_cast<void*>(static_cast<std::uintptr_t>(node.id()));
    // Неактивные узлы бледные, как в Unity.
    const bool inactive = !node.activeInHierarchy();
    if (inactive) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }
    const bool open = ImGui::TreeNodeEx(id, flags, "%s  %s", icons::forNodeType(node.typeName()), node.name().c_str());
    if (inactive) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("%s", std::string(node.typeName()).c_str());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
        if (ImGui::GetIO().KeyCtrl) {
            selection.toggle(node.id());
        } else {
            selection.select(node.id());
        }
        m_reveal = false;
    }
    if (ImGui::BeginPopupContextItem()) {
        // Меню на одном из выбранных относится ко всему выбору.
        if (!selection.isSelected(node.id())) {
            selection.select(node.id());
        }
        if (ImGui::MenuItem(tr("editor.tree.addChild").c_str())) {
            m_addNode.open(node.id());
        }
        if (ImGui::MenuItem(tr("editor.tree.delete").c_str(), "Delete")) {
            m_deleteRequested = true;
        }
        ImGui::EndPopup();
    }
    if (selected && m_reveal) {
        ImGui::SetScrollHereY();
    }

    if (open) {
        for (const auto& child : node.children()) {
            drawNode(*child, selection);
        }
        ImGui::TreePop();
    }
}

} // namespace yqlora::editor
