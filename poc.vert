#version 450

layout(push_constant) uniform upc {
  vec2 extent;
};

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 i_pos;
layout(location = 2) in vec2 i_size;
layout(location = 3) in vec4 i_colour;

layout(location = 0) out vec4 f_colour;

void main() {
  vec2 p = (i_pos + pos * i_size) / extent;
  gl_Position = vec4(p * 2.0 - 1.0, 0, 1);
  f_colour = i_colour;
}

