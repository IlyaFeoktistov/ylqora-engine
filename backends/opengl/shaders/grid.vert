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
