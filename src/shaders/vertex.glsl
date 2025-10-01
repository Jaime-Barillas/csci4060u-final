#version 450

layout(location = 0) in vec3 position;
layout(std140, set = 1, binding = 0) uniform Transforms {
    mat4 projection;
};

void main() {
    gl_Position = projection * vec4(position, 1.0);
}
