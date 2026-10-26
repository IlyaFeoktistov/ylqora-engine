#include "ViewportPanel.hpp"

#include "Theme.hpp"
#include "gizmos/AxisGizmo.hpp"
#include "gizmos/CameraGizmo.hpp"
#include "gizmos/GridGizmo.hpp"
#include "gizmos/ManipulatorGizmo.hpp"
#include "gizmos/SelectionGizmo.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/render/Renderer.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <imgui.h>

#include <string>

namespace yqlora {

ViewportPanel::ViewportPanel()
{
    // Порядок - приоритет мыши: манипулятор и оси раньше выбора кликом.
    m_gizmos.add(std::make_unique<ManipulatorGizmo>());
    m_gizmos.add(std::make_unique<AxisGizmo>());
    m_gizmos.add(std::make_unique<CameraGizmo>());
    m_gizmos.add(std::make_unique<SelectionGizmo>());
    m_gizmos.add(std::make_unique<GridGizmo>());
}

void ViewportPanel::draw(Application& app, EditorSession& session, float deltaSeconds)
{
    Scene& scene = session.scene();

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
    const bool hovered = ImGui::IsWindowHovered()
                         && ImGui::IsMouseHoveringRect(origin, ImVec2(origin.x + size.x, origin.y + size.y));

    // Игра показывается через свою камеру, и инструменты редактора ей не нужны.
    const bool playing = session.playing();
    const Camera3D* gameCamera = playing ? scene.activeCamera() : nullptr;
    m_gizmos.setEnabled(!playing);

    m_camera.update(app.input(), deltaSeconds, hovered && !gameCamera && !m_mouseOverGizmo);
    setFlying(m_camera.flying());

    const float aspect = size.x / size.y;
    GizmoContext context{
        .session = session,
        .camera = m_camera,
        .view = m_camera.cameraData(aspect, app.depthRange()),
        .origin = origin,
        .size = size,
        .mouseFree = hovered && !m_camera.flying(),
    };
    m_mouseOverGizmo = m_gizmos.draw(context);
    if (playing) {
        drawPlayModeFrame(origin, size, gameCamera != nullptr);
    }

    // Текстура уже стоит в списке отрисовки интерфейса, а нарисуется он позже,
    // так что кадр сцены успевает попасть в неё в этом же кадре.
    FrameData frame = buildFrameData(scene, aspect, app.depthRange());
    if (!gameCamera) {
        frame.camera = m_camera.cameraData(aspect, app.depthRange());
    }
    m_gizmos.prepareFrame(frame, session);
    app.renderer().render(frame, *m_target);

    ImGui::End();
}

void ViewportPanel::drawPlayModeFrame(const ImVec2& origin, const ImVec2& size, bool hasCamera)
{
    ImDrawList& list = *ImGui::GetWindowDrawList();
    list.AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), theme::toImU32(theme::colors().playMode), 0.0f,
                 0, 2.0f);
    if (!hasCamera) {
        // Без активной камеры игре не через что показываться: смотрим через камеру редактора.
        const std::string text = tr("editor.play.noCamera");
        list.AddText(ImVec2(origin.x + 12.0f, origin.y + size.y - ImGui::GetTextLineHeight() - 10.0f),
                     ImGui::GetColorU32(ImGuiCol_Text), text.c_str());
    }
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
