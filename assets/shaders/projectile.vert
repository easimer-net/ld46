#version 330 core
layout (location = 0) in vec2 vPosition;
layout (location = 1) in vec3 vColor;
layout (location = 2) in float flTTL;

out vec4 outColor;

uniform mat4 matMVP;

void main() {
    gl_Position = matMVP * vec4(vPosition, 0.0, 1.0);
    float flAlpha = clamp(flTTL / 0.5, 0, 1);
    float flBright = 1.25 - pow(flAlpha, 3);
    outColor = vec4(flBright * vColor, flAlpha);
}
