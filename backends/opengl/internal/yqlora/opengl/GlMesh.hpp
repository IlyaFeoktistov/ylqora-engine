#pragma once

#include "yqlora/render/GpuResourceCache.hpp"

namespace yqlora {
struct DrawItem;
struct MeshData;
} // namespace yqlora

namespace yqlora::opengl {

// Меш, загруженный в видеопамять.
class GlMesh {
public:
    explicit GlMesh(const MeshData& data);
    ~GlMesh();

    GlMesh(const GlMesh&) = delete;
    GlMesh& operator=(const GlMesh&) = delete;

    void draw() const;

private:
    unsigned int m_vertexArray = 0;
    unsigned int m_vertexBuffer = 0;
    unsigned int m_indexBuffer = 0;
    int m_indexCount = 0;
};

// Копии мешей кадра в видеопамяти, см. GpuResourceCache.
class GlMeshCache {
public:
    const GlMesh& get(const DrawItem& item);

    void endFrame()
    {
        m_meshes.endFrame();
    }

private:
    GpuResourceCache<GlMesh> m_meshes;
};

} // namespace yqlora::opengl
