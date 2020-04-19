#version 330 core
layout (location = 0) in vec2 vPosition;

uniform mat4 matMVP;

void main() {
    gl_Position = matMVP * vec4(vPosition, 0.0, 1.0);
}
