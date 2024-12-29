#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColour;

layout(location = 0) out vec4 outColour;

void main() {
  gl_Position = vec4(inPosition, 0.0, 1.0);
  outColour = vec4(inColour, 1.0);
}
