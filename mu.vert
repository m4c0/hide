#version 450

layout(push_constant) uniform upc {
  float aspect;
};

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 ipos;
layout(location = 2) in vec2 size;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec4 colour;

layout(location = 0) out vec4 f_colour;
layout(location = 1) out vec2 f_uv;

void main() {
  vec2 p = (ipos + pos * size - 200) / (vec2(aspect, 1) * 200);
  gl_Position = vec4(p, 0, 1);
  f_colour = pow(colour, vec4(2.2));
  f_uv = (uv + pos / 16.0) * step(1e-9, length(uv));
}
