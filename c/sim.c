#include <SDL3/SDL_stdinc.h>
#include <cglm/struct.h>
#include "sim.h"

void init_particles(SimCtx *ctx) {
  int64_t margin = 50;
  int64_t side_count = (int64_t)SDL_ceilf(SDL_sqrtf(ctx->pcount));
  int64_t spacing_x = (ctx->window_width - (2 * margin)) / side_count;
  int64_t spacing_y = (ctx->window_height - (2 * margin)) / side_count;
  for (int64_t i = 0; i < ctx->pcount; i++) {
    int64_t x = i % side_count;
    int64_t y = i / side_count;
    ctx->ps[i].pos.x = (x * spacing_x) + (spacing_x / 2.0) + margin;
    ctx->ps[i].pos.y = (y * spacing_y) + (spacing_y / 2.0) + margin;
    ctx->ps[i].vel.x = 0;
    ctx->ps[i].vel.y = 0;
  }
}

