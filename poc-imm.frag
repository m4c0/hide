#version 450

layout(set = 0, binding = 0) uniform sampler2D image;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 mult;

layout(location = 0) out vec4 colour;

void main() {
  colour = texture(image, uv) * mult;
}
