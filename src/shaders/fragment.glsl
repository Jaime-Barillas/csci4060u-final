#version 450

const float PI = 3.14159265359;

// Note: Set 3 as per SDL_CreateGPUShader.
layout(std140, set = 3, binding = 0) uniform Params {
    uvec2 bounds;
    int sphere_count;
    float sphere_radius;
} params;

// Note: Set 2 as per SDL_CreateGPUShader.
layout(std430, set = 2, binding = 0) readonly buffer SphereBuffer {
    vec3 positions[];
} spheres;

layout(location = 0) out vec4 colour;

float origin_z(float view_width, float degrees) {
    /* Calculate the distance the camera must be from the origin to have a
     * horizontal view angle of `degrees`.
     *  ─┼─── view_width ────┤
     *   │╲angle1  ╎  angle1╱
     *   │ ╲       ╎       ╱ By law of sines:
     *   │  ╲      ╎      ╱       view_width * sin(angle1)
     *   │an-╲     ╎     ╱   ?? = ────────────────────────
     *   │gle ╲    ╎    ╱             2 * sin(angle2)
     *   │2    ╲degrees╱
     *   │      ╲  ╎  ╱
     * ??┤    an-╲ ╎ ╱
     *   │    gle1╲╎╱
     *  ─┼────┬────┤
     *  view_width / 2
     */
    const float angle1 = (PI - radians(degrees)) / 2.0;
    const float angle2 = (PI / 2.0) - angle1;
    return (view_width * sin(angle1)) / (2 * sin(angle2));
}

void main() {
    // Normalize fragcoord to [-1, +1].
    vec2 bounds = vec2(params.bounds);
    vec3 frag = vec3((gl_FragCoord.xy / bounds * 2.0) - 1.0, 0.0);
    // Expand horizontal bounds by the aspect ratio to ensure image is not stretched.
    // And flip y axis so positive is upwards.
    frag.x *= bounds.x / bounds.y;
    frag.y *= -1;

    vec3 origin = vec3(0.0, 0.0, -origin_z(2.0, 70.0));
    const vec3 dir = normalize(frag - origin);
    origin.z -= 1.25;


    // Seed with large value.
    float t = 999.0;
    int hit_index;
    for (int i = 0; i < params.sphere_count; i++) {
        float r1;
        float r2;
        //    a                b                      c
        // (dir^2) t^2 + (2 dot(orig, dir)) t + (orig^2 - R^2)
        vec3 offset = origin - spheres.positions[i];
        float a = dot(dir, dir);
        float b = 2.0 * dot(offset, dir);
        float c = dot(offset, offset) - (params.sphere_radius * params.sphere_radius);

        // TODO: https://stackoverflow.com/a/50065711 ?
        // (-b +/- sqrt(b^2 - 4ac))
        // ------------------------
        //           2a
        float descriminant = (b * b) - (4 * a * c);
        if (descriminant < -0.001) {
            continue;
        } else if (descriminant < 0.001) {
            r1 = -b / (2 * a);
            r2 = r1;
        } else {
            float a2 = 2 * a;
            r1 = (-b + sqrt(descriminant)) / a2;
            r2 = (-b - sqrt(descriminant)) / a2;
        }

        // Skip if sphere is behind us, or we are inside the sphere.
        if (r1 < 0 || r2 < 0) { continue; }
        r1 = (r1 < r2) ? r1 : r2;
        if (r1 < t) {
            t = r1;
            hit_index = i;
        }
    }

    // Diffuse lighting only.
    vec3 hit = origin + (dir * t);
    vec3 norm = normalize(hit - spheres.positions[hit_index]);
    const vec3 LIGHT = normalize(vec3(0.2, 0.5, -1.0));
    float di = clamp(dot(norm, LIGHT), 0.0, 1.0);

    if (t < 999.0) {
        colour = vec4(1.0 * di, 1.0 * di, 1.0 * di, 1.0);
    } else {
        colour = vec4(0.05, 0.05, 0.25, 1.0);
    }
}
