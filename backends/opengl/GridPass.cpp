#include "GridPass.hpp"

#include "GeneratedShaders.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace yqlora {

namespace {

// Квадрат из двух треугольников, у каждой вершины две координаты.
constexpr int kCornerComponents = 2;
constexpr int kCornerCount = 6;

// Шаг мелкой сетки в метрах; каждая десятая линия крупная.
constexpr float kCellSize = 1.0f;

// Дальше этого расстояния от камеры сетки не видно. Это же половина стороны квадрата.
constexpr float kFadeDistance = 150.0f;

} // namespace

GridPass::GridPass()
    : m_shader("grid", shaders::kGridVert, shaders::kGridFrag)
{
    // Два треугольника на квадрат [-1, 1], размер и положение задаёт вершинный шейдер.
    // clang-format off
    const float corners[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
    };
    // clang-format on

    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, kCornerComponents, GL_FLOAT, GL_FALSE, kCornerComponents * sizeof(float), nullptr);
    glBindVertexArray(0);

    m_viewProjectionLocation = m_shader.uniformLocation("uViewProjection");
    m_cameraPositionLocation = m_shader.uniformLocation("uCameraPosition");
    m_cellSizeLocation = m_shader.uniformLocation("uCellSize");
    m_fadeDistanceLocation = m_shader.uniformLocation("uFadeDistance");
}

GridPass::~GridPass()
{
    glDeleteBuffers(1, &m_vertexBuffer);
    glDeleteVertexArrays(1, &m_vertexArray);
}

void GridPass::draw(const CameraData& camera) const
{
    const Mat4 viewProjection = camera.projection * camera.view;

    m_shader.use();
    glUniformMatrix4fv(m_viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniform3fv(m_cameraPositionLocation, 1, glm::value_ptr(camera.position));
    glUniform1f(m_cellSizeLocation, kCellSize);
    glUniform1f(m_fadeDistanceLocation, kFadeDistance);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    // Снизу сетку тоже видно.
    glDisable(GL_CULL_FACE);

    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, kCornerCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace yqlora
