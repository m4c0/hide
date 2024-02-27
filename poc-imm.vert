#version 450

layout(push_constant) uniform upc {
  float aspect;
};

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 i_pos;
layout(location = 2) in vec2 i_size;

void main() {
  vec2 p = i_pos + pos * i_size;
  p = p / vec2(aspect, 1);
  gl_Position = vec4(p, 0, 1);
}
