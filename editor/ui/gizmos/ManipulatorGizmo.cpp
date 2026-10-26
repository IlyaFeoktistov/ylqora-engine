#include "ManipulatorGizmo.hpp"

#include "Icons.hpp"

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/TransformEdit.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/scene/Node3D.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
// ImGuizmo.h использует типы ImGui и должен идти после imgui.h.
#include <ImGuizmo.h>

namespace yqlora {

bool ManipulatorGizmo::draw(GizmoContext& context)
{
    const bool toolbarHovered = drawToolbar(context);
    if (context.mouseFree && !context.camera.flying()) {
        handleHotkeys();
    }

    return drawManipulator(context) || toolbarHovered;
}

bool ManipulatorGizmo::drawToolbar(const GizmoContext& context)
{
    struct Button {
        Tool tool;
        const char* icon;
        const char* labelKey;
        const char* hotkey;
    };
    static constexpr Button kButtons[] = {
        {Tool::Select, icons::kSelect, "editor.tool.select", "Q"},
        {Tool::Move, icons::kMove, "editor.tool.move", "W"},
        {Tool::Rotate, icons::kRotate, "editor.tool.rotate", "E"},
        {Tool::Scale, icons::kScale, "editor.tool.scale", "R"},
    };

    ImGui::SetCursorScreenPos(ImVec2(context.origin.x + 8.0f, context.origin.y + 8.0f));
    bool hovered = false;
    for (const Button& button : kButtons) {
        const bool active = m_tool == button.tool;
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(button.icon)) {
            m_tool = button.tool;
        }
        if (active) {
            ImGui::PopStyleColor();
        }
        hovered = hovered || ImGui::IsItemHovered();
        ImGui::SetItemTooltip("%s (%s)", tr(button.labelKey).c_str(), button.hotkey);
        ImGui::SameLine(0.0f, 2.0f);
    }

    ImGui::SameLine(0.0f, 12.0f);
    if (ImGui::Button(m_localSpace ? icons::kLocal : icons::kGlobal)) {
        m_localSpace = !m_localSpace;
    }
    hovered = hovered || ImGui::IsItemHovered();
    ImGui::SetItemTooltip("%s: %s", tr(m_localSpace ? "editor.tool.local" : "editor.tool.global").c_str(),
                          tr("editor.tool.spaceHint").c_str());

    return hovered;
}

void ManipulatorGizmo::handleHotkeys()
{
    // Пока печатают в поле ввода, буквы - это текст, а не инструменты.
    if (ImGui::GetIO().WantTextInput) {
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
        m_tool = Tool::Select;
    } else if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        m_tool = Tool::Move;
    } else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        m_tool = Tool::Rotate;
    } else if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        m_tool = Tool::Scale;
    }
}

bool ManipulatorGizmo::drawManipulator(GizmoContext& context)
{
    auto* node = dynamic_cast<Node3D*>(context.session.selectedNode());
    if (!node || m_tool == Tool::Select || context.camera.flying()) {
        return false;
    }

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
    switch (m_tool) {
    case Tool::Rotate:
        operation = ImGuizmo::ROTATE;
        break;
    case Tool::Scale:
        operation = ImGuizmo::SCALE;
        break;
    case Tool::Select:
    case Tool::Move:
        break;
    }
    // Масштаб в ImGuizmo бывает только по своим осям объекта.
    const ImGuizmo::MODE mode = m_localSpace || m_tool == Tool::Scale ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    // Доля высоты окна, которую занимает манипулятор. Стандартный (0.1) мелковат.
    constexpr float kManipulatorSize = 0.15f;
    ImGuizmo::SetGizmoSizeClipSpace(kManipulatorSize);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(context.origin.x, context.origin.y, context.size.x, context.size.y);
    ImGuizmo::SetOrthographic(false);

    Mat4 world = node->worldMatrix();
    if (ImGuizmo::Manipulate(glm::value_ptr(context.view.view), glm::value_ptr(context.view.projection), operation,
                             mode, glm::value_ptr(world))) {
        setWorldMatrix(*node, world);
        context.session.markModified();
    }

    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}

} // namespace yqlora
