#include "ManipulatorGizmo.hpp"

#include "style/Icons.hpp"

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/scene/Node3D.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
// ImGuizmo.h использует типы ImGui и должен идти после imgui.h.
#include <ImGuizmo.h>

namespace yqlora::editor {

namespace {

// Панель инструментов в углу окна сцены.
constexpr float kToolbarMargin = 8.0f;
constexpr float kButtonSpacing = 2.0f;
// Между инструментами и переключателем осей.
constexpr float kGroupSpacing = 12.0f;

} // namespace

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

    ImGui::SetCursorScreenPos(ImVec2(context.origin.x + kToolbarMargin, context.origin.y + kToolbarMargin));
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
        ImGui::SameLine(0.0f, kButtonSpacing);
    }

    ImGui::SameLine(0.0f, kGroupSpacing);
    if (ImGui::Button(m_localSpace ? icons::kLocal : icons::kGlobal)) {
        m_localSpace = !m_localSpace;
    }
    hovered = hovered || ImGui::IsItemHovered();
    ImGui::SetItemTooltip(
        "%s: %s",
        tr(m_localSpace ? "editor.tool.local" : "editor.tool.global").c_str(),
        tr("editor.tool.spaceHint").c_str()
    );

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
    case Tool::Rotate: operation = ImGuizmo::ROTATE; break;
    case Tool::Scale: operation = ImGuizmo::SCALE; break;
    case Tool::Select:
    case Tool::Move: break;
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
    const bool moved = ImGuizmo::Manipulate(
        glm::value_ptr(context.view.view),
        glm::value_ptr(context.view.projection),
        operation,
        mode,
        glm::value_ptr(world)
    );
    if (moved) {
        if (const auto values = node->localValuesForWorld(world)) {
            context.session.setTransform(*node, *values);
        }
    }

    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}

} // namespace yqlora::editor
