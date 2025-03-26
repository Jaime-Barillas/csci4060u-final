#ifndef SIM_H
#define SIM_H

#include <stdint.h>
#include <cglm/struct.h>

typedef struct Particle_S {
  vec2s pos;
  vec2s vel;
} Particle;

typedef struct SimCtx_S {
  vec2s     gravity;
  int64_t   interaction_radius;
  int64_t   sim_steps;

  int64_t   window_width;
  int64_t   window_height;
  Particle *ps;
  Particle *ps2;
  int64_t   pcount;
  int64_t   cell_size;
  int64_t   grid_width;
  int64_t   grid_height;
  int64_t   cell_count;
  int64_t  *cell_pcount;

  float     rest_density;
  float     pressure_mult;
  float     collision_damping;
  float     time_step;
} SimCtx;

void init_particles(SimCtx *ctx);
void sim_step(SimCtx *ctx, float dt);

#endif
