#include "yqlora/opengl/GlTexture.hpp"

#include "yqlora/render/TextureData.hpp"

#include <glad/gl.h>

namespace yqlora::opengl {

namespace {

GLint wrapMode(TextureWrap wrap)
{
    switch (wrap) {
    case TextureWrap::Repeat: return GL_REPEAT;
    case TextureWrap::Clamp: return GL_CLAMP_TO_EDGE;
    case TextureWrap::Mirror: return GL_MIRRORED_REPEAT;
    }

    return GL_REPEAT;
}

GLint minFilter(const TextureSampling& sampling)
{
    if (sampling.filter == TextureFilter::Nearest) {
        return sampling.mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
    }

    return sampling.mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
}

} // namespace

GlTexture::GlTexture(const TextureData& data, Use use)
{
    const TextureSampling& sampling = data.sampling;
    const bool srgb = use == Use::Scene && sampling.colorSpace == ColorSpace::Srgb;
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    // Строки идут сверху вниз и попадают в текстуру первой строкой при v = 0:
    // это и есть uv0 = (0, 0) в левом верхнем углу, переворачивать не нужно.
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8,
        data.width,
        data.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data.pixels.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode(sampling.wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode(sampling.wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter(sampling));
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        sampling.filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR
    );
    if (sampling.mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

GlTexture::~GlTexture()
{
    glDeleteTextures(1, &m_texture);
}

} // namespace yqlora::opengl
