#include "GridPass.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace yqlora {

namespace {

// Шаг мелкой сетки в метрах; каждая десятая линия крупная.
constexpr float kCellSize = 1.0f;

// Дальше этого расстояния от камеры сетки не видно. Это же половина стороны квадрата.
constexpr float kFadeDistance = 150.0f;

constexpr const char* kVertexShader = R"(
#version 330 core

layout(location = 0) in vec2 aCorner;

uniform mat4 uViewProjection;
uniform vec3 uCameraPosition;
uniform float uFadeDistance;

out vec3 vWorld;

void main()
{
    vWorld = vec3(uCameraPosition.x + aCorner.x * uFadeDistance, 0.0,
                  uCameraPosition.z + aCorner.y * uFadeDistance);
    gl_Position = uViewProjection * vec4(vWorld, 1.0);
}
)";

constexpr const char* kFragmentShader = R"(
#version 330 core

in vec3 vWorld;

uniform vec3 uCameraPosition;
uniform float uCellSize;
uniform float uFadeDistance;

out vec4 fragColor;

// 1 на линии сетки с шагом cell, 0 между линиями. Толщина линии около пикселя
// на любом расстоянии: fwidth говорит, сколько единиц мира приходится на пиксель.
float gridLines(vec2 coord, float cell)
{
    vec2 scaled = coord / cell;
    vec2 distanceToLine = abs(fract(scaled - 0.5) - 0.5) / fwidth(scaled);

    return 1.0 - min(min(distanceToLine.x, distanceToLine.y), 1.0);
}

float axisLine(float coord)
{
    return 1.0 - min(abs(coord) / (fwidth(coord) * 1.5), 1.0);
}

void main()
{
    float distanceToCamera = length(vWorld - uCameraPosition);
    float fade = 1.0 - smoothstep(uFadeDistance * 0.3, uFadeDistance, distanceToCamera);
    // Мелкие клетки гаснут раньше: вдали они сливаются в рябь.
    float minorFade = 1.0 - smoothstep(uFadeDistance * 0.1, uFadeDistance * 0.4, distanceToCamera);

    float minor = gridLines(vWorld.xz, uCellSize) * minorFade;
    float major = gridLines(vWorld.xz, uCellSize * 10.0);
    vec4 color = vec4(0.5, 0.5, 0.5, max(minor * 0.25, major * 0.5));

    // Ось X лежит на z = 0, ось Z на x = 0. Цвета как у гизмо: X красный, Z синий.
    color = mix(color, vec4(0.9, 0.3, 0.3, 0.9), axisLine(vWorld.z));
    color = mix(color, vec4(0.3, 0.45, 0.95, 0.9), axisLine(vWorld.x));

    color.a *= fade;
    if (color.a < 0.002) {
        discard;
    }
    fragColor = color;
}
)";

} // namespace

GridPass::GridPass()
    : m_shader("grid", kVertexShader, kFragmentShader)
{
    // Два треугольника на квадрат [-1, 1], размер и положение задаёт вершинный шейдер.
    const float corners[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
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
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace yqlora
