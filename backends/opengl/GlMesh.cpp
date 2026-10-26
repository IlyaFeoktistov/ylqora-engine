#include "GlMesh.hpp"

#include "yqlora/render/MeshData.hpp"

#include <glad/gl.h>

#include <cstddef>

namespace yqlora {

GlMesh::GlMesh(const MeshData& data)
    : m_indexCount(static_cast<int>(data.indices.size()))
{
    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glGenBuffers(1, &m_indexBuffer);

    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex)),
                 data.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.indices.size() * sizeof(std::uint32_t)),
                 data.indices.data(), GL_STATIC_DRAW);

    // location 0 - положение, 1 - нормаль. Так их ждут все шейдеры мешей.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, normal)));
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

const GlMesh& GlMeshCache::get(const MeshData& data)
{
    auto& mesh = m_meshes[&data];
    if (!mesh) {
        mesh = std::make_unique<GlMesh>(data);
    }

    return *mesh;
}

} // namespace yqlora
