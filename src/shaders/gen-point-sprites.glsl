#version 450

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430, set = 0, binding = 0) readonly buffer ParticleBuffer {
    vec3 pos[];
} particles;

// NOTE: Triangle list.
layout(std430, set = 1, binding = 0) buffer VertexBuffer {
    vec3 pos[];
} vertices;

const unsigned int VERTICES_PER_QUAD = 6;
const vec3 VERT_BL_OFFSET = vec3(-0.1, -0.1, 0.0);
const vec3 VERT_TL_OFFSET = vec3(-0.1,  0.1, 0.0);
const vec3 VERT_TR_OFFSET = vec3( 0.1,  0.1, 0.0);
const vec3 VERT_BR_OFFSET = vec3( 0.1, -0.1, 0.0);

void main() {
    unsigned int particle_idx = gl_GlobalInvocationID.x;
    unsigned int vertex_base = particle_idx * VERTICES_PER_QUAD;

    vec3 vert_bl = particles.pos[particle_idx] + VERT_BL_OFFSET;
    vec3 vert_tl = particles.pos[particle_idx] + VERT_TL_OFFSET;
    vec3 vert_tr = particles.pos[particle_idx] + VERT_TR_OFFSET;
    vec3 vert_br = particles.pos[particle_idx] + VERT_BR_OFFSET;

    vertices.pos[vertex_base    ] = vert_bl;
    vertices.pos[vertex_base + 1] = vert_tl;
    vertices.pos[vertex_base + 2] = vert_tr;

    vertices.pos[vertex_base + 3] = vert_tr;
    vertices.pos[vertex_base + 4] = vert_br;
    vertices.pos[vertex_base + 5] = vert_bl;
}

