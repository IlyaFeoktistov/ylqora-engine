#pragma once

#include "yqlora/render/RenderTarget.hpp"

namespace yqlora {

// Framebuffer с цветом в текстуре и своим буфером глубины.
class GlRenderTarget final : public RenderTarget {
public:
    GlRenderTarget(int width, int height);
    ~GlRenderTarget() override;

    GlRenderTarget(const GlRenderTarget&) = delete;
    GlRenderTarget& operator=(const GlRenderTarget&) = delete;

    int width() const override
    {
        return m_width;
    }

    int height() const override
    {
        return m_height;
    }

    void resize(int width, int height) override;
    UiTexture uiTexture() const override;

    unsigned int framebuffer() const
    {
        return m_framebuffer;
    }

private:
    void allocate();

    unsigned int m_framebuffer = 0;
    unsigned int m_colorTexture = 0;
    unsigned int m_depthBuffer = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace yqlora
