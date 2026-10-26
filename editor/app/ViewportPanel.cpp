#include "ViewportPanel.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/Picking.hpp"
#include "yqlora/editor/Selection.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/render/Renderer.hpp"

#include <imgui.h>

#include <string>

namespace yqlora {

void ViewportPanel::draw(Application& app, Scene& scene, Selection& selection, float deltaSeconds)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const std::string title = tr("editor.window.viewport") + "###Viewport";
    const bool visible = ImGui::Begin(title.c_str());
    ImGui::PopStyleVar();

    const ImVec2 size = ImGui::GetContentRegionAvail();
    if (!visible || size.x < 1.0f || size.y < 1.0f) {
        ImGui::End();
        setFlying(app, false);

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
    const bool hovered = ImGui::IsItemHovered();

    m_camera.update(app.input(), deltaSeconds, hovered);
    setFlying(app, m_camera.flying());

    if (hovered && !m_camera.flying() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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
    app.renderer().render(frame, *m_target);

    ImGui::End();
}

void ViewportPanel::setFlying(Application& app, bool flying)
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
    app.setMouseWrapping(flying);
}

} // namespace yqlora
