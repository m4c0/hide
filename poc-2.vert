#version 450

layout(constant_id = 0) const float aspect = 1.6f;

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 ipos;
layout(location = 2) in vec4 colour;

layout(location = 0) out vec4 f_colour;

void main() {
  vec2 p = (ipos + pos) / (vec2(aspect, 1) * 16);
  gl_Position = vec4(p, 0, 1);
  f_colour = colour;
}
