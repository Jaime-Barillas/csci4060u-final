#version 450

layout(location = 0) in vec2 tex_coord;
layout(location = 0) out vec4 colour;

const vec2 tex_center = vec2(0.5, 0.5);

void main() {
    // The particle extends in the shape of a circle to the edges of its quad.
    // We need to discard anything outside the circle.
    if (length(tex_coord - tex_center) > 0.5) {
        discard;
    }
    colour = vec4(tex_coord.xy, 0.0, 1.0);
}
