#include "panels/ViewportPanel.hpp"

#include "gizmos/AxisGizmo.hpp"
#include "gizmos/CameraGizmo.hpp"
#include "gizmos/GridGizmo.hpp"
#include "gizmos/ManipulatorGizmo.hpp"
#include "gizmos/SelectionGizmo.hpp"
#include "style/Theme.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/render/Renderer.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <imgui.h>

#include <string>

namespace yqlora::editor {

namespace {

// Наискосок сверху, как вид по умолчанию в Blender: видно все три оси сразу.
const Vec3 kStartCameraPosition(6.0f, 4.5f, 6.0f);
const Vec3 kStartCameraTarget(0.0f, 0.5f, 0.0f);
constexpr float kPlayFrameThickness = 2.0f;
// Подсказка в левом нижнем углу окна сцены.
constexpr ImVec2 kHintMargin(12.0f, 10.0f);

} // namespace

ViewportPanel::ViewportPanel()
{
    // Порядок - приоритет мыши: манипулятор и оси раньше выбора кликом.
    m_gizmos.add(std::make_unique<ManipulatorGizmo>());
    m_gizmos.add(std::make_unique<AxisGizmo>());
    m_gizmos.add(std::make_unique<CameraGizmo>());
    m_gizmos.add(std::make_unique<SelectionGizmo>());
    m_gizmos.add(std::make_unique<GridGizmo>());
}

void ViewportPanel::onProjectOpened(EditorContext& /*context*/)
{
    m_camera.setPosition(kStartCameraPosition);
    m_camera.lookAt(kStartCameraTarget);
}

void ViewportPanel::draw(EditorContext& context)
{
    EditorSession& session = context.session;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = ImGui::Begin(title().c_str());
    ImGui::PopStyleVar();

    const ImVec2 size = ImGui::GetContentRegionAvail();
    if (!visible || size.x < 1.0f || size.y < 1.0f) {
        ImGui::End();
        setFlying(false);

        return;
    }

    const ViewArea area = drawSceneImage(context.app, size);
    // Игра показывается через свою камеру, и инструменты редактора ей не нужны.
    const bool playing = session.playing();
    const Camera3D* gameCamera = playing ? session.scene().activeCamera() : nullptr;
    m_gizmos.setEnabled(!playing);

    m_camera.update(context.app.input(), context.deltaSeconds, area.hovered && !gameCamera && !m_mouseOverGizmo);
    setFlying(m_camera.flying());
    context.mouseWrapping = context.mouseWrapping || m_camera.flying();

    const float aspect = size.x / size.y;
    GizmoContext gizmoContext{
        .session = session,
        .camera = m_camera,
        .theme = context.theme,
        .view = m_camera.cameraData(aspect, context.app.depthRange()),
        .origin = area.origin,
        .size = size,
        .mouseFree = area.hovered && !m_camera.flying(),
    };
    m_mouseOverGizmo = m_gizmos.draw(gizmoContext);
    if (playing) {
        drawPlayModeFrame(area.origin, size, gameCamera != nullptr, context.theme.colors());
    }

    renderScene(context, aspect, gameCamera != nullptr, gizmoContext);
    ImGui::End();
}

ViewportPanel::ViewArea ViewportPanel::drawSceneImage(Application& app, const ImVec2& size)
{
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
    ImGui::Image(
        ImTextureRef(texture.id),
        size,
        ImVec2(texture.uv0.x, texture.uv0.y),
        ImVec2(texture.uv1.x, texture.uv1.y)
    );
    const bool hovered = ImGui::IsWindowHovered()
                         && ImGui::IsMouseHoveringRect(origin, ImVec2(origin.x + size.x, origin.y + size.y));

    return {.origin = origin, .hovered = hovered};
}

void ViewportPanel::renderScene(
    EditorContext& context,
    float aspect,
    bool throughGameCamera,
    const GizmoContext& gizmoContext
)
{
    Application& app = context.app;
    // Текстура уже стоит в списке отрисовки интерфейса, а нарисуется он позже,
    // так что кадр сцены успевает попасть в неё в этом же кадре.
    FrameData frame = buildFrameData(context.session.scene(), aspect, app.depthRange());
    if (!throughGameCamera) {
        frame.camera = m_camera.cameraData(aspect, app.depthRange());
    }
    m_gizmos.prepareFrame(frame, gizmoContext);
    app.renderer().render(frame, *m_target);
}

void ViewportPanel::drawPlayModeFrame(
    const ImVec2& origin,
    const ImVec2& size,
    bool hasCamera,
    const EditorColors& colors
)
{
    ImDrawList& list = *ImGui::GetWindowDrawList();
    list.AddRect(
        origin,
        ImVec2(origin.x + size.x, origin.y + size.y),
        toImU32(colors.playMode),
        0.0f,
        0,
        kPlayFrameThickness
    );
    if (!hasCamera) {
        // Без активной камеры игре не через что показываться: смотрим через камеру редактора.
        const std::string text = tr("editor.play.noCamera");
        list.AddText(
            ImVec2(origin.x + kHintMargin.x, origin.y + size.y - ImGui::GetTextLineHeight() - kHintMargin.y),
            ImGui::GetColorU32(ImGuiCol_Text),
            text.c_str()
        );
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

} // namespace yqlora::editor
