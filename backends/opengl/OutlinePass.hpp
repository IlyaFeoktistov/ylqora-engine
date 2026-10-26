#pragma once

#include "GlShader.hpp"

#include "yqlora/render/FrameData.hpp"

namespace yqlora {

class GlMeshCache;

// Контур выбранного узла в редакторе. Сначала узел рисуется белым в маску
// размером с окно, затем полноэкранный проход закрашивает пиксели рядом
// с маской. Толщина контура в пикселях не зависит от расстояния до объекта,
// а сам контур виден и сквозь другие объекты.
class OutlinePass {
public:
    explicit OutlinePass(GlMeshCache& meshes);
    ~OutlinePass();

    OutlinePass(const OutlinePass&) = delete;
    OutlinePass& operator=(const OutlinePass&) = delete;

    // Рисует в текущий framebuffer размером width x height.
    void draw(const FrameData& frame, int width, int height);

private:
    void resizeMask(int width, int height);

    GlMeshCache& m_meshes;
    GlShader m_maskShader;
    GlShader m_outlineShader;
    unsigned int m_maskFramebuffer = 0;
    unsigned int m_maskTexture = 0;
    unsigned int m_emptyVertexArray = 0;
    int m_maskWidth = 0;
    int m_maskHeight = 0;
    int m_maskTransformLocation = -1;
    int m_outlineMaskLocation = -1;
    int m_outlineColorLocation = -1;
};

} // namespace yqlora
