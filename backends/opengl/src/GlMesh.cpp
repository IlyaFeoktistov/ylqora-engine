#include "yqlora/opengl/GlMesh.hpp"

#include "yqlora/render/FrameData.hpp"
#include "yqlora/render/MeshData.hpp"

#include <glad/gl.h>

#include <cstddef>

namespace yqlora::opengl {

namespace {

// Так входы вершин ждут все шейдеры мешей.
constexpr GLuint kPositionLocation = 0;
constexpr GLuint kNormalLocation = 1;
constexpr GLuint kUv0Location = 2;

} // namespace

GlMesh::GlMesh(const MeshData& data)
    : m_indexCount(static_cast<int>(data.indices.size()))
{
    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glGenBuffers(1, &m_indexBuffer);

    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex)),
        data.vertices.data(),
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(data.indices.size() * sizeof(std::uint32_t)),
        data.indices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(kPositionLocation);
    glVertexAttribPointer(
        kPositionLocation,
        Vec3::length(),
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(kNormalLocation);
    glVertexAttribPointer(
        kNormalLocation,
        Vec3::length(),
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, normal))
    );
    glEnableVertexAttribArray(kUv0Location);
    glVertexAttribPointer(
        kUv0Location,
        Vec2::length(),
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, uv0))
    );
    glBindVertexArray(0);
}

GlMesh::~GlMesh()
{
    glDeleteBuffers(1, &m_indexBuffer);
    glDeleteBuffers(1, &m_vertexBuffer);
    glDeleteVertexArrays(1, &m_vertexArray);
}

void GlMesh::draw() const
{
    glBindVertexArray(m_vertexArray);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

const GlMesh& GlMeshCache::get(const DrawItem& item)
{
    // Команды OpenGL идут по порядку: буферы старой копии можно удалить, даже
    // если ими уже рисовали в этом кадре.
    return m_meshes.get(item.meshId, item.meshRevision, [&] {
        return std::make_unique<GlMesh>(*item.mesh);
    });
}

} // namespace yqlora::opengl
