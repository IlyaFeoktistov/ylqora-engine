#include "ViewportPanel.hpp"

#include "Theme.hpp"
#include "ViewportGizmos.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/Picking.hpp"
#include "yqlora/editor/TransformEdit.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/render/Renderer.hpp"
#include "yqlora/scene/Node3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
// ImGuizmo.h использует типы ImGui и должен идти после imgui.h.
#include <ImGuizmo.h>

#include <string>

namespace yqlora {

void ViewportPanel::draw(Application& app, EditorSession& session, float deltaSeconds)
{
    Scene& scene = session.scene();
    Selection& selection = session.selection();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const std::string title = tr("editor.window.viewport") + "###Viewport";
    const bool visible = ImGui::Begin(title.c_str());
    ImGui::PopStyleVar();

    const ImVec2 size = ImGui::GetContentRegionAvail();
    if (!visible || size.x < 1.0f || size.y < 1.0f) {
        ImGui::End();
        setFlying(false);

        return;
    }

    // Окно в точках, текстура в пикселях: на экранах с масштабом их больше.
    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    const int width = static_cast<int>(size.x * scale.x);
    const int height = static_cast<int>(size.y * scale.y);
    if (!m_target) {
        m_target = app.renderer().createRenderTarget(width, height);
    }
    m_target->resize(width, height);

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const UiTexture texture = m_target->uiTexture();
    ImGui::Image(ImTextureRef(texture.id), size, ImVec2(texture.uv0.x, texture.uv0.y),
                 ImVec2(texture.uv1.x, texture.uv1.y));
    const bool toolbarHovered = drawToolbar(origin);
    const bool hovered = ImGui::IsWindowHovered() && !toolbarHovered
                         && ImGui::IsMouseHoveringRect(origin, ImVec2(origin.x + size.x, origin.y + size.y));
    if (hovered && !m_camera.flying()) {
        handleToolHotkeys();
    }

    m_camera.update(app.input(), deltaSeconds, hovered);
    setFlying(m_camera.flying());

    // Камера уже на новом месте: рисунки поверх кадра строим по ней.
    const CameraData view = m_camera.cameraData(size.x / size.y, app.depthRange());
    if (const Node* root = scene.root()) {
        gizmos::drawCameras(*root, selection.selected(), view, origin, size);
    }
    const bool manipulatorActive = drawManipulator(session, view, origin, size);
    const gizmos::AxisGizmoResult axes = gizmos::drawAxes(view.view, origin, size);
    if (axes.lookAlong) {
        m_camera.lookAlong(*axes.lookAlong);
    }

    const bool overWidget = axes.hovered || manipulatorActive;
    if (hovered && !overWidget && !m_camera.flying() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        const Ray ray = m_camera.screenRay({mouse.x - origin.x, mouse.y - origin.y}, {size.x, size.y});
        selection.select(pickNode(scene, ray));
    }

    // Текстура уже стоит в списке отрисовки интерфейса, а нарисуется он позже,
    // так что кадр сцены успевает попасть в неё в этом же кадре.
    FrameData frame = buildFrameData(scene, size.x / size.y, app.depthRange());
    frame.camera = m_camera.cameraData(size.x / size.y, app.depthRange());
    frame.editor.grid = true;
    frame.editor.selected = selection.selected();
    frame.editor.outlineColor = theme::colors().selectionOutline;
    app.renderer().render(frame, *m_target);

    ImGui::End();
}

bool ViewportPanel::drawToolbar(const ImVec2& origin)
{
    struct Button {
        Tool tool;
        const char* labelKey;
        const char* hotkey;
    };
    static constexpr Button kButtons[] = {
        {Tool::Select, "editor.tool.select", "Q"},
        {Tool::Move, "editor.tool.move", "W"},
        {Tool::Rotate, "editor.tool.rotate", "E"},
        {Tool::Scale, "editor.tool.scale", "R"},
    };

    const ImVec2 start(origin.x + 8.0f, origin.y + 8.0f);
    ImGui::SetCursorScreenPos(start);
    bool toolbarHovered = false;
    for (const Button& button : kButtons) {
        const bool active = m_tool == button.tool;
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(tr(button.labelKey).c_str())) {
            m_tool = button.tool;
        }
        if (active) {
            ImGui::PopStyleColor();
        }
        toolbarHovered = toolbarHovered || ImGui::IsItemHovered();
        ImGui::SetItemTooltip("%s (%s)", tr(button.labelKey).c_str(), button.hotkey);
        ImGui::SameLine(0.0f, 2.0f);
    }

    ImGui::SameLine(0.0f, 12.0f);
    if (ImGui::Button(tr(m_localSpace ? "editor.tool.local" : "editor.tool.global").c_str())) {
        m_localSpace = !m_localSpace;
    }
    toolbarHovered = toolbarHovered || ImGui::IsItemHovered();
    ImGui::SetItemTooltip("%s", tr("editor.tool.spaceHint").c_str());

    return toolbarHovered;
}

void ViewportPanel::handleToolHotkeys()
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

bool ViewportPanel::drawManipulator(EditorSession& session, const CameraData& view, const ImVec2& origin,
                                    const ImVec2& size)
{
    auto* node = dynamic_cast<Node3D*>(session.selectedNode());
    if (!node || m_tool == Tool::Select || m_camera.flying()) {
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
    ImGuizmo::SetRect(origin.x, origin.y, size.x, size.y);
    ImGuizmo::SetOrthographic(false);

    Mat4 world = node->worldMatrix();
    if (ImGuizmo::Manipulate(glm::value_ptr(view.view), glm::value_ptr(view.projection), operation, mode,
                             glm::value_ptr(world))) {
        setWorldMatrix(*node, world);
        session.markModified();
    }

    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}

void ViewportPanel::setFlying(bool flying)
{
    ImGuiIO& io = ImGui::GetIO();
    const bool wasFlying = (io.ConfigFlags & ImGuiConfigFlags_NoMouse) != 0;
    if (flying == wasFlying) {
        return;
    }

    // В полёте мышь и клавиатура принадлежат камере: интерфейс не должен
    // подсвечивать кнопки, через которые пролетает курсор, и печатать WASD в поля ввода.
    const ImGuiConfigFlags captured = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange
                                      | ImGuiConfigFlags_NoKeyboard;
    if (flying) {
        io.ConfigFlags |= captured;
    } else {
        io.ConfigFlags &= ~captured;
    }
}

} // namespace yqlora
