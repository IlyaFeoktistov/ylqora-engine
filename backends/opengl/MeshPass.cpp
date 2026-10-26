#include "MeshPass.hpp"

#include "GlMesh.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace yqlora {

namespace {

constexpr const char* kVertexShader = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uViewProjection;
uniform mat4 uModel;
uniform mat3 uNormalMatrix;

out vec3 vNormal;

void main()
{
    vNormal = uNormalMatrix * aNormal;
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
}
)";

constexpr const char* kFragmentShader = R"(
#version 330 core

in vec3 vNormal;

uniform vec4 uColor;
uniform vec3 uLightDirection;

out vec4 fragColor;

void main()
{
    // Рассеянный свет плюс постоянная подсветка, чтобы теневые стороны не были чёрными.
    float diffuse = max(dot(normalize(vNormal), -uLightDirection), 0.0);
    float light = 0.35 + 0.65 * diffuse;
    fragColor = vec4(uColor.rgb * light, uColor.a);
}
)";

// Свет падает сверху, немного сбоку и спереди.
const Vec3 kLightDirection = glm::normalize(Vec3(-0.4f, -1.0f, -0.3f));

} // namespace

MeshPass::MeshPass(GlMeshCache& meshes)
    : m_meshes(meshes)
    , m_shader("mesh", kVertexShader, kFragmentShader)
{
    m_viewProjectionLocation = m_shader.uniformLocation("uViewProjection");
    m_modelLocation = m_shader.uniformLocation("uModel");
    m_normalMatrixLocation = m_shader.uniformLocation("uNormalMatrix");
    m_colorLocation = m_shader.uniformLocation("uColor");
    m_lightDirectionLocation = m_shader.uniformLocation("uLightDirection");
}

void MeshPass::draw(const FrameData& frame) const
{
    if (frame.drawItems.empty()) {
        return;
    }
    const Mat4 viewProjection = frame.camera.projection * frame.camera.view;

    m_shader.use();
    glUniformMatrix4fv(m_viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniform3fv(m_lightDirectionLocation, 1, glm::value_ptr(kLightDirection));

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    for (const DrawItem& item : frame.drawItems) {
        // Нормали поворачиваются вместе с объектом, но неравномерный масштаб
        // их бы исказил: для этого и нужна обратная транспонированная матрица.
        const Mat3 normalMatrix = glm::transpose(glm::inverse(Mat3(item.world)));
        glUniformMatrix4fv(m_modelLocation, 1, GL_FALSE, glm::value_ptr(item.world));
        glUniformMatrix3fv(m_normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniform4fv(m_colorLocation, 1, glm::value_ptr(item.color));
        m_meshes.get(*item.mesh).draw();
    }
    glDisable(GL_CULL_FACE);
}

} // namespace yqlora
