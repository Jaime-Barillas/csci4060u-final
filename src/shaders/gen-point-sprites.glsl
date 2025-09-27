#version 450

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std140, set = 0, binding = 0) readonly buffer ParticleBuffer {
    vec3 pos[];
} particles;

layout(std140, set = 1, binding = 0) buffer VertexBuffer {
    vec3 pos[];
} vertices;

int posCount = 1000;
int vertCount = 6000;
float r = 0.1;

void main() {
    // TODO
}

