#include "yqlora/editor/ui/widgets/RenderView.hpp"

#include "yqlora/render/Renderer.hpp"

namespace yqlora::editor {

bool RenderView::resize(Renderer& renderer, const ImVec2& size)
{
    // Окно в точках, текстура в пикселях: на экранах с масштабом их больше.
    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    const int width = static_cast<int>(size.x * scale.x);
    const int height = static_cast<int>(size.y * scale.y);
    if (width < 1 || height < 1) {
        return false;
    }
    if (!m_target) {
        m_target = renderer.createRenderTarget(width, height);
    }
    m_target->resize(width, height);

    return true;
}

void RenderView::image(const ImVec2& size) const
{
    if (!m_target) {
        ImGui::Dummy(size);

        return;
    }
    const UiTexture texture = m_target->uiTexture();
    ImGui::Image(
        ImTextureRef(texture.id),
        size,
        ImVec2(texture.uv0.x, texture.uv0.y),
        ImVec2(texture.uv1.x, texture.uv1.y)
    );
}

void RenderView::image(ImDrawList& list, const ImVec2& min, const ImVec2& max) const
{
    if (!m_target) {
        return;
    }
    const UiTexture texture = m_target->uiTexture();
    list.AddImage(
        ImTextureRef(texture.id),
        min,
        max,
        ImVec2(texture.uv0.x, texture.uv0.y),
        ImVec2(texture.uv1.x, texture.uv1.y)
    );
}

void RenderView::render(Renderer& renderer, const FrameData& frame)
{
    if (m_target) {
        renderer.render(frame, *m_target);
    }
}

} // namespace yqlora::editor
