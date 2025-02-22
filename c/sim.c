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

void sim_step(SimCtx *ctx, float dt) {
  vec2s old_pos;
  for (int64_t i = 0; i < ctx->pcount; i++) {
    // 2. Apply gravity to velocities.
    ctx->ps[i].vel = glms_vec2_add(
      ctx->ps[i].vel,
      glms_vec2_scale(ctx->gravity, dt)
    );

    // 4. Advance to predicted position.
    {
      // Saving old position for next velocity computation.
      old_pos = ctx->ps[i].pos;
      ctx->ps[i].pos = glms_vec2_add(
        ctx->ps[i].pos,
        glms_vec2_scale(ctx->ps[i].vel, dt)
      );
    }

    // 8. Resolve collisions.
    {
      // FIXME: Add linear repulsive force.
      // Clamp to window bounds.
      ctx->ps[i].pos.x = SDL_clamp(ctx->ps[i].pos.x, 0, ctx->window_width - 1);
      ctx->ps[i].pos.y = SDL_clamp(ctx->ps[i].pos.y, 0, ctx->window_height - 1);
    }

    // 9. Compute next velocity.
    {
      ctx->ps[i].vel = glms_vec2_divs(
        glms_vec2_sub(ctx->ps[i].pos, old_pos),
        dt
      );
    }
  }
}

