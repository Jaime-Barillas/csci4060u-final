#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord_in;
layout(std140, set = 1, binding = 0) uniform Transforms {
    mat4 projection;
};

layout(location = 0) out vec2 tex_coord;

void main() {
    tex_coord = tex_coord_in;
    gl_Position = projection * vec4(position, 1.0);
}
