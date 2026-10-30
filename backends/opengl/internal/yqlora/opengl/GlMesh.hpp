#pragma once

#include <map>
#include <memory>

namespace yqlora {
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

// Загружает меш в видеопамять при первом обращении и дальше отдаёт готовый.
// Ключ: указатель на MeshData, он живёт до конца программы.
class GlMeshCache {
public:
    const GlMesh& get(const MeshData& data);

private:
    std::map<const MeshData*, std::unique_ptr<GlMesh>> m_meshes;
};

} // namespace yqlora::opengl
