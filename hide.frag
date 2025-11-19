#version 450

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec4 f_colour;

layout(location = 0) out vec4 colour;

void main() {
  colour = f_colour;
}
