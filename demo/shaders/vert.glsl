#version 450

layout(location = 0) out vec4 fragColour;

vec2 positions[3] = vec2[](
  vec2(0.0, -0.5), // Top
  vec2(0.5, 0.5), // Bottom right
  vec2(-0.5, 0.5) // Bottom left
);

vec3 colours[3] = vec3[](
  vec3(1.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0)
);

void main() {
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  fragColour = vec4(colours[gl_VertexIndex], 1.0);
}
