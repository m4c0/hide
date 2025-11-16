#version 450

layout(constant_id = 0) const float aspect = 1.6f;

layout(location = 0) in vec2 pos;

void main() {
  vec2 p = pos / vec2(aspect, 1);
  gl_Position = vec4(p, 0, 1);
}
