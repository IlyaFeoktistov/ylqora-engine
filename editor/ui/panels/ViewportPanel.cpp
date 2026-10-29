#include "panels/ViewportPanel.hpp"

#include "gizmos/AxisGizmo.hpp"
#include "gizmos/CameraGizmo.hpp"
#include "gizmos/GridGizmo.hpp"
#include "gizmos/ManipulatorGizmo.hpp"
#include "gizmos/SelectionGizmo.hpp"
#include "style/Icons.hpp"
#include "style/Theme.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/EditorSettings.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/render/Renderer.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/RenderSystem.hpp"
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
// Превью камеры: доля ширины окна сцены и отступ от его угла.
constexpr float kPreviewShare = 0.3f;
constexpr float kPreviewMargin = 12.0f;
constexpr float kPreviewBorder = 1.0f;
// Кнопка превью: тот же отступ от угла, что у панели инструментов слева.
constexpr float kToggleMargin = 8.0f;

// Пиксели текстуры для области в точках: на экранах с масштабом их больше.
ImVec2 toPixels(const ImVec2& size)
{
    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;

    return {size.x * scale.x, size.y * scale.y};
}

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
    const RenderSystem* render = session.scene().system<RenderSystem>();
    const Camera3D* gameCamera = playing && render ? render->mainCamera() : nullptr;
    m_gizmos.setEnabled(!playing);

    // Превью и его кнопка лежат поверх сцены: клики по ним до сцены не доходят.
    const bool overToggle = !gameCamera && drawPreviewToggle(context, area.origin, size);
    const Camera3D* previewed = previewedCamera(context, gameCamera != nullptr);
    const Rect preview = previewRect(area.origin, size);
    const bool overPreview = previewed && preview.hovered();
    const bool sceneHovered = area.hovered && !overToggle && !overPreview;

    m_camera.update(context.app.input(), context.deltaSeconds, sceneHovered && !gameCamera && !m_mouseOverGizmo);
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
        .mouseFree = sceneHovered && !m_camera.flying(),
    };
    m_mouseOverGizmo = m_gizmos.draw(gizmoContext);
    if (playing) {
        drawPlayModeFrame(area.origin, size, gameCamera != nullptr, context.theme.colors());
    }

    renderScene(context, aspect, gameCamera != nullptr, gizmoContext);
    if (previewed) {
        drawCameraPreview(context, *previewed, preview);
    }
    ImGui::End();
}

ViewportPanel::ViewArea ViewportPanel::drawSceneImage(Application& app, const ImVec2& size)
{
    const ImVec2 pixels = toPixels(size);
    const int width = static_cast<int>(pixels.x);
    const int height = static_cast<int>(pixels.y);
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
    FrameData frame = extractFrame(context.session.scene(), aspect, app.depthRange());
    if (!throughGameCamera) {
        frame.views = {{
            .camera = m_camera.cameraData(aspect, app.depthRange()),
            .background = context.theme.colors().viewportBackground,
        }};
    }
    m_gizmos.prepareFrame(frame, gizmoContext);
    app.renderer().render(frame, *m_target);
}

bool ViewportPanel::Rect::hovered() const
{
    return ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(min, ImVec2(min.x + size.x, min.y + size.y));
}

const Camera3D* ViewportPanel::previewedCamera(EditorContext& context, bool throughGameCamera) const
{
    if (!context.settings.cameraPreview || throughGameCamera) {
        return nullptr;
    }

    return dynamic_cast<const Camera3D*>(context.session.selectedNode());
}

ViewportPanel::Rect ViewportPanel::previewRect(const ImVec2& origin, const ImVec2& size)
{
    const ImVec2 previewSize(size.x * kPreviewShare, size.y * kPreviewShare);

    return {
        .min = ImVec2(
            origin.x + size.x - previewSize.x - kPreviewMargin,
            origin.y + size.y - previewSize.y - kPreviewMargin
        ),
        .size = previewSize,
    };
}

bool ViewportPanel::drawPreviewToggle(EditorContext& context, const ImVec2& origin, const ImVec2& size)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float width = ImGui::CalcTextSize(icons::kCamera3D).x + style.FramePadding.x * 2.0f;
    ImGui::SetCursorScreenPos(ImVec2(origin.x + size.x - width - kToggleMargin, origin.y + kToggleMargin));

    bool& enabled = context.settings.cameraPreview;
    const bool wasEnabled = enabled;
    if (wasEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    }
    if (ImGui::Button(icons::kCamera3D)) {
        enabled = !enabled;
        context.settings.save();
    }
    if (wasEnabled) {
        ImGui::PopStyleColor();
    }
    const bool hovered = ImGui::IsItemHovered();
    ImGui::SetItemTooltip("%s", tr("editor.viewport.cameraPreview").c_str());

    return hovered;
}

void ViewportPanel::drawCameraPreview(EditorContext& context, const Camera3D& camera, const Rect& rect)
{
    Application& app = context.app;
    const ImVec2 pixels = toPixels(rect.size);
    const int width = static_cast<int>(pixels.x);
    const int height = static_cast<int>(pixels.y);
    if (width < 1 || height < 1) {
        return;
    }
    if (!m_previewTarget) {
        m_previewTarget = app.renderer().createRenderTarget(width, height);
    }
    m_previewTarget->resize(width, height);

    // Только эта камера, как её увидит игра: без гизмо и без остальных камер.
    const float aspect = rect.size.x / rect.size.y;
    FrameData frame = extractFrame(context.session.scene(), aspect, app.depthRange());
    frame.views = {cameraView(camera, aspect, app.depthRange())};
    app.renderer().render(frame, *m_previewTarget);

    const ImVec2 max(rect.min.x + rect.size.x, rect.min.y + rect.size.y);
    const UiTexture texture = m_previewTarget->uiTexture();
    ImDrawList& list = *ImGui::GetWindowDrawList();
    list.AddImage(
        ImTextureRef(texture.id),
        rect.min,
        max,
        ImVec2(texture.uv0.x, texture.uv0.y),
        ImVec2(texture.uv1.x, texture.uv1.y)
    );
    list.AddRect(rect.min, max, toImU32(context.theme.colors().cameraGizmo), 0.0f, 0, kPreviewBorder);
    list.AddText(
        ImVec2(rect.min.x + kPreviewBorder, rect.min.y - ImGui::GetTextLineHeightWithSpacing()),
        ImGui::GetColorU32(ImGuiCol_Text),
        camera.name().c_str()
    );
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
        // Без камеры игре не через что показываться: смотрим через камеру редактора.
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
