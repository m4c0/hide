#version 450

layout(push_constant) uniform upc {
  vec2 extent;
};

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 i_pos;

void main() {
  vec2 p = (i_pos + pos) / extent;
  gl_Position = vec4(p * 2.0 - 1.0, 0, 1);
}

