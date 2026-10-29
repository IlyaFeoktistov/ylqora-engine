#pragma once

#include "GlShader.hpp"

#include "yqlora/render/FrameData.hpp"

namespace yqlora {

class GlMeshCache;

// Непрозрачные меши кадра. Пока источников света в сцене нет, всё освещено
// одним фиксированным направленным светом.
class MeshPass {
public:
    explicit MeshPass(GlMeshCache& meshes);

    // Меши кадра со слоем из маски вида, через камеру вида.
    void draw(const FrameData& frame, const ViewData& view) const;

private:
    GlMeshCache& m_meshes;
    GlShader m_shader;
    int m_viewProjectionLocation = -1;
    int m_modelLocation = -1;
    int m_normalMatrixLocation = -1;
    int m_colorLocation = -1;
    int m_lightDirectionLocation = -1;
};

} // namespace yqlora
