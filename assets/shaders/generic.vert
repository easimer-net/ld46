#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;

out vec4 outColor;
out vec2 vTexcoord;

uniform mat4 matMVP;

void main() {
    gl_Position = matMVP * vec4(vPosition.xy, 0.0, 1.0);
    vTexcoord = vUV;
    outColor = vec4(0.5, 0.0, 0.0, 1.0);
}
