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
  int64_t   pcount;
  int64_t   cell_size;
  int64_t   grid_width;
  int64_t   grid_height;
  int64_t   cell_count;
  int64_t  *cell_pcount;
} SimCtx;

void init_particles(SimCtx *ctx);

#endif
