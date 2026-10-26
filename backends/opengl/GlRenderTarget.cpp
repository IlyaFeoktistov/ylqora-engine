#include "GlRenderTarget.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <stdexcept>

namespace yqlora {

GlRenderTarget::GlRenderTarget(int width, int height)
    : m_width(std::max(width, 1))
    , m_height(std::max(height, 1))
{
    glGenFramebuffers(1, &m_framebuffer);
    glGenTextures(1, &m_colorTexture);
    glGenRenderbuffers(1, &m_depthBuffer);
    allocate();
}

GlRenderTarget::~GlRenderTarget()
{
    glDeleteRenderbuffers(1, &m_depthBuffer);
    glDeleteTextures(1, &m_colorTexture);
    glDeleteFramebuffers(1, &m_framebuffer);
}

void GlRenderTarget::resize(int width, int height)
{
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (width == m_width && height == m_height) {
        return;
    }
    m_width = width;
    m_height = height;
    allocate();
}

UiTexture GlRenderTarget::uiTexture() const
{
    // OpenGL хранит первую строку текстуры внизу, интерфейс ждёт её сверху.
    return {m_colorTexture, {0.0f, 1.0f}, {1.0f, 0.0f}};
}

void GlRenderTarget::allocate()
{
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Render target framebuffer is incomplete");
    }
}

} // namespace yqlora
