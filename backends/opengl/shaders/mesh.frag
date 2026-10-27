#version 330 core

in vec3 vNormal;

uniform vec4 uColor;
uniform vec3 uLightDirection;

out vec4 fragColor;

void main()
{
    // Рассеянный свет плюс постоянная подсветка, чтобы теневые стороны не были чёрными.
    const float kAmbient = 0.35;
    float diffuse = max(dot(normalize(vNormal), -uLightDirection), 0.0);
    float light = kAmbient + (1.0 - kAmbient) * diffuse;
    fragColor = vec4(uColor.rgb * light, uColor.a);
}
