#version 450

layout(location = 0) out vec4 colour;
layout(location = 1) in vec4 frag_colour;

void main() {
    colour = frag_colour;
}
