#version 450

layout(push_constant) uniform upc {
  float aspect;
};

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 i_pos;
layout(location = 2) in vec2 i_size;
layout(location = 3) in vec2 i_uv_pos;
layout(location = 4) in vec2 i_uv_size;

layout(location = 0) out vec2 f_uv;

void main() {
  vec2 p = i_pos + pos * i_size;
  p = p / vec2(aspect, 1);
  gl_Position = vec4(p, 0, 1);
  f_uv = i_uv_pos + pos * i_uv_size;
}
