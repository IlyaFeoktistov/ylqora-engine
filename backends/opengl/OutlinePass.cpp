#include "OutlinePass.hpp"

#include "GeneratedShaders.hpp"
#include "GlMesh.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

namespace yqlora {

namespace {

// Один треугольник на весь экран: вершины строит сам шейдер по gl_VertexID.
constexpr int kFullscreenTriangleVertices = 3;

} // namespace

OutlinePass::OutlinePass(GlMeshCache& meshes)
    : m_meshes(meshes)
    , m_maskShader("outline mask", shaders::kOutlineMaskVert, shaders::kOutlineMaskFrag)
    , m_outlineShader("outline", shaders::kFullscreenVert, shaders::kOutlineFrag)
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

void OutlinePass::draw(const FrameData& frame, const CameraData& camera, int width, int height)
{
    const auto isSelected = [&](const DrawItem& item) {
        return std::ranges::find(frame.editor.selected, item.node) != frame.editor.selected.end();
    };
    const bool hasSelectedMesh = std::ranges::any_of(frame.drawItems, isSelected);
    if (!hasSelectedMesh || width <= 0 || height <= 0) {
        return;
    }
    resizeMask(width, height);

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    // 1. Маска выбранных узлов, без теста глубины: контур виден сквозь другие объекты.
    glBindFramebuffer(GL_FRAMEBUFFER, m_maskFramebuffer);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    const Mat4 viewProjection = camera.projection * camera.view;
    m_maskShader.use();
    for (const DrawItem& item : frame.drawItems) {
        if (!isSelected(item)) {
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
    glDrawArrays(GL_TRIANGLES, 0, kFullscreenTriangleVertices);
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
