#include "OutlinePass.hpp"

#include "GlMesh.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

namespace yqlora {

namespace {

constexpr const char* kMaskVertexShader = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uTransform;

void main()
{
    gl_Position = uTransform * vec4(aPosition, 1.0);
}
)";

constexpr const char* kMaskFragmentShader = R"(
#version 330 core

out vec4 fragColor;

void main()
{
    fragColor = vec4(1.0);
}
)";

// Треугольник на весь экран без вершинного буфера: координаты из gl_VertexID.
constexpr const char* kFullscreenVertexShader = R"(
#version 330 core

out vec2 vUv;

void main()
{
    vUv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vUv * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr const char* kOutlineFragmentShader = R"(
#version 330 core

in vec2 vUv;

uniform sampler2D uMask;
uniform vec4 uColor;

out vec4 fragColor;

void main()
{
    // Внутри объекта контур не рисуем.
    if (texture(uMask, vUv).r > 0.5) {
        discard;
    }

    // Пиксель снаружи, но маска рядом, в радиусе двух пикселей: это контур.
    vec2 texel = 1.0 / vec2(textureSize(uMask, 0));
    float nearMask = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            if (x * x + y * y <= 5) {
                nearMask = max(nearMask, texture(uMask, vUv + vec2(x, y) * texel).r);
            }
        }
    }
    if (nearMask < 0.5) {
        discard;
    }
    fragColor = uColor;
}
)";

} // namespace

OutlinePass::OutlinePass(GlMeshCache& meshes)
    : m_meshes(meshes)
    , m_maskShader("outline mask", kMaskVertexShader, kMaskFragmentShader)
    , m_outlineShader("outline", kFullscreenVertexShader, kOutlineFragmentShader)
{
    m_maskTransformLocation = m_maskShader.uniformLocation("uTransform");
    m_outlineMaskLocation = m_outlineShader.uniformLocation("uMask");
    m_outlineColorLocation = m_outlineShader.uniformLocation("uColor");

    glGenFramebuffers(1, &m_maskFramebuffer);
    glGenTextures(1, &m_maskTexture);
    // Core profile не рисует без привязанного VAO, даже если атрибутов нет.
    glGenVertexArrays(1, &m_emptyVertexArray);
}

OutlinePass::~OutlinePass()
{
    glDeleteVertexArrays(1, &m_emptyVertexArray);
    glDeleteTextures(1, &m_maskTexture);
    glDeleteFramebuffers(1, &m_maskFramebuffer);
}

void OutlinePass::draw(const FrameData& frame, int width, int height)
{
    const NodeId selected = frame.editor.selected;
    const bool hasSelectedMesh = selected != 0 && std::ranges::any_of(frame.drawItems, [&](const DrawItem& item) {
        return item.node == selected;
    });
    if (!hasSelectedMesh || width <= 0 || height <= 0) {
        return;
    }
    resizeMask(width, height);

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    // 1. Маска выбранного узла, без теста глубины: контур виден сквозь другие объекты.
    glBindFramebuffer(GL_FRAMEBUFFER, m_maskFramebuffer);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    const Mat4 viewProjection = frame.camera.projection * frame.camera.view;
    m_maskShader.use();
    for (const DrawItem& item : frame.drawItems) {
        if (item.node != selected) {
            continue;
        }
        const Mat4 transform = viewProjection * item.world;
        glUniformMatrix4fv(m_maskTransformLocation, 1, GL_FALSE, glm::value_ptr(transform));
        m_meshes.get(*item.mesh).draw();
    }

    // 2. Контур поверх кадра.
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
    glViewport(0, 0, width, height);
    m_outlineShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_maskTexture);
    glUniform1i(m_outlineMaskLocation, 0);
    glUniform4fv(m_outlineColorLocation, 1, glm::value_ptr(frame.editor.outlineColor));
    glBindVertexArray(m_emptyVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void OutlinePass::resizeMask(int width, int height)
{
    if (width == m_maskWidth && height == m_maskHeight) {
        return;
    }
    m_maskWidth = width;
    m_maskHeight = height;

    glBindTexture(GL_TEXTURE_2D, m_maskTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_maskFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_maskTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
}

} // namespace yqlora
