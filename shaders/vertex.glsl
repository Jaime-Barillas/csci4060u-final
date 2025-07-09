#version 450

layout(location = 0) in vec3 p;
layout(location = 1) out vec4 frag_colour;

vec4 colours[3] = vec4[](
    vec4( 1.0,  0.0,  0.0, 1.0),
    vec4( 0.0,  1.0,  0.0, 1.0),
    vec4( 0.0,  0.0,  1.0, 1.0)
);

void main() {
    gl_Position = vec4(p, 1.0);
    frag_colour = colours[gl_VertexIndex % 3];
}
