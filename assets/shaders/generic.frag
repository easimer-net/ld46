#version 330 core
out vec4 FragColor;
  
in vec4 outColor;
in vec2 vTexcoord;

uniform sampler2D tex0;

void main() {
    FragColor = texture(tex0, vTexcoord);
}
