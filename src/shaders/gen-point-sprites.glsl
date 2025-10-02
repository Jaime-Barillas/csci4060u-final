#version 450

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430, set = 0, binding = 0) readonly buffer ParticleBuffer {
    vec3 pos[];
    // float padding;
} particles;

// NOTE: Triangle list.
layout(std430, set = 1, binding = 0) buffer VertexBuffer {
    vec4 pos[];
    // float padding;
} vertices;
layout(std430, set = 1, binding = 1) buffer TexCoordBuffer {
    vec2 coord[];
} coords;

layout(std140, set = 2, binding = 0) uniform TransformBuffer {
    mat4 view_model;
};

const uint VERTICES_PER_QUAD = 6;
const vec4 VERT_BL_OFFSET = vec4(-0.1, -0.1, 0.0, 0.0);
const vec4 VERT_TL_OFFSET = vec4(-0.1,  0.1, 0.0, 0.0);
const vec4 VERT_TR_OFFSET = vec4( 0.1,  0.1, 0.0, 0.0);
const vec4 VERT_BR_OFFSET = vec4( 0.1, -0.1, 0.0, 0.0);

const vec2 TEX_COORD_TL = vec2(0.0, 0.0);
const vec2 TEX_COORD_TR = vec2(1.0, 0.0);
const vec2 TEX_COORD_BL = vec2(0.0, 1.0);
const vec2 TEX_COORD_BR = vec2(1.0, 1.0);

void main() {
    uint particle_idx = gl_GlobalInvocationID.x;
    uint vertex_base = particle_idx * VERTICES_PER_QUAD;

    vec4 particle_pos = view_model * vec4(particles.pos[particle_idx], 1.0);
    vec4 vert_bl = particle_pos + VERT_BL_OFFSET;
    vec4 vert_tl = particle_pos + VERT_TL_OFFSET;
    vec4 vert_tr = particle_pos + VERT_TR_OFFSET;
    vec4 vert_br = particle_pos + VERT_BR_OFFSET;

    // Triangle 1.
    vertices.pos[vertex_base    ] = vert_bl;
    vertices.pos[vertex_base + 1] = vert_tl;
    vertices.pos[vertex_base + 2] = vert_tr;

    coords.coord[vertex_base    ] = TEX_COORD_BL;
    coords.coord[vertex_base + 1] = TEX_COORD_TL;
    coords.coord[vertex_base + 2] = TEX_COORD_TR;

    // Triangle 2.
    vertices.pos[vertex_base + 3] = vert_tr;
    vertices.pos[vertex_base + 4] = vert_br;
    vertices.pos[vertex_base + 5] = vert_bl;

    coords.coord[vertex_base + 3] = TEX_COORD_TR;
    coords.coord[vertex_base + 4] = TEX_COORD_BR;
    coords.coord[vertex_base + 5] = TEX_COORD_BL;
}

