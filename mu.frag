#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec4 f_colour;
layout(location = 1) in vec2 f_uv;

layout(location = 0) out vec4 colour;

void main() {
  float a = mix(1, texture(tex, f_uv).r, step(1e-9, length(f_uv)));
  colour = f_colour * a;
}
