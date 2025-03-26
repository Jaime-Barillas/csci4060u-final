#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_log.h>
#include <cglm/struct.h>
#include "sim.h"
#include "util.h"

void cell_coords(int64_t cell_index, int64_t *x, int64_t *y, SimCtx *ctx) {
  *x = cell_index % ctx->grid_width;
  *y = cell_index / ctx->grid_width;
}

void  cell_index(int64_t x, int64_t y, int64_t *idx, SimCtx *ctx) {
  *idx = (y * ctx->grid_width) + x;
}

void cell_bounds(int64_t cell_index, int64_t *start, int64_t *end, SimCtx *ctx) {
  *start = ctx->cell_pcount[cell_index] - 1;
  if (cell_index >= (ctx->cell_count - 1)) {
    *end = ctx->pcount;
  } else {
    *end = ctx->cell_pcount[cell_index + 1] - 1;
  }
}

bool valid_non_empty_cell(int64_t cell_index, int64_t cell_x, int64_t cell_y, SimCtx *ctx) {
  return (cell_x >= 0) && (cell_x < ctx->grid_width) &&
         (cell_y >= 0) && (cell_y < ctx->grid_height) &&
         ctx->cell_pcount[cell_index] != 0;
}

void init_particles(SimCtx *ctx) {
  float aspect_ratio = (float)ctx->window_width / ctx->window_height;
  int64_t along_x = (int64_t)SDL_ceilf(SDL_sqrtf(ctx->pcount * aspect_ratio));
  int64_t along_y = ctx->pcount / along_x;
  int64_t spacing = 12; // FIXME: Hard-coded value.
  for (int64_t i = 0; i < ctx->pcount; i++) {
    int64_t x = i % along_x;
    int64_t y = i / along_x;
    ctx->ps[i].pos.x = (ctx->window_width / 2.0) - (along_x * spacing / 2.0) + (x * spacing);
    ctx->ps[i].pos.y = (ctx->window_height / 2.0) - (along_y * spacing / 2.0) + (y * spacing);
    ctx->ps[i].vel.x = 0;
    ctx->ps[i].vel.y = 0;
  }
}

void sim_step(SimCtx *ctx, float dt) {
  // 1. Sort particles by grid cell.
  //sort_by_cell(ctx);

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

    // 7. Apply Double Density Relaxation.
    {
      float density = 0;
      float pressure = 0;

      int64_t cell_x, cell_y, cell_idx;
      cell_coords(cell_for_pos(ctx->ps[i].pos.x, ctx->ps[i].pos.y, ctx), &cell_x, &cell_y, ctx);

      // Calculate pressures.
      float volume = 3.141592654 * 2 * ctx->interaction_radius / 3.0;
      // for (int64_t y = cell_y - 1; y < cell_y + 1; y++) {
        // for (int64_t x = cell_x - 1; x < cell_x + 1; x++) {
          // cell_index(x, y, &cell_idx, ctx);
          // if (!valid_non_empty_cell(cell_idx, x, y, ctx)) {
          //   continue;
          // }

          // int64_t start, end;
          // cell_bounds(cell_idx, &start, &end, ctx);
          // for (int64_t j = start; j < end; j++) {
          //   if (i == j) { continue; } // Skip self

          for (int64_t j = 0; j < ctx->pcount; j++) {
            if (i == j) {continue;}
            vec2s r = glms_vec2_sub(ctx->ps[j].pos, ctx->ps[i].pos);
            float kern = 1.0 - (glms_vec2_norm(r) / ctx->interaction_radius);
            // Consider only particles closer than the interaction radius.
            if (kern > 0) {
              density += kern * kern / volume;
            }
            }
          // }
        // }
      // }

      pressure = ctx->pressure_mult * (density - ctx->rest_density);
    vec2s pos = ctx->ps[i].pos;
    if (i == 25) SDL_Log("\x1b[0Kp25: (%.4f, %.4f) cell: (%ld, %ld)", pressure, density, cell_x, cell_y);
    if (i == 25) SDL_Log("\x1b[0Kp25 pos: %.1f, %.1f - idx: %ld", pos.x, pos.y, cell_for_pos(pos.x, pos.y, ctx));
    if (i == 26) SDL_Log("\x1b[0Kp26: (%.4f, %.4f) cell: (%ld, %ld)", pressure, density, cell_x, cell_y);

      // Calculate displacements.
      vec2s d = glms_vec2_zero();
    //   for (int64_t y = cell_y - 1; y < cell_y + 1; y++) {
    //     for (int64_t x = cell_x - 1; x < cell_x + 1; x++) {
    //       cell_index(x, y, &cell_idx, ctx);
    //       if (!valid_non_empty_cell(cell_idx, x, y, ctx)) {
    //         continue;
    //       }
    // if (i == 25) SDL_Log("\x1b[0Kp: %ld (%ld, %ld)", cell_idx, x, y);

    //       int64_t start, end;
    //       cell_bounds(cell_idx, &start, &end, ctx);
    //       for (int64_t j = start; j < end; j++) {
          for (int64_t j = 0; j < ctx->pcount; j++) {
            if (i == j) { continue; } // Skip self

            vec2s r = glms_vec2_sub(ctx->ps[j].pos, ctx->ps[i].pos);
            float r_norm = glms_vec2_norm(r);
            float kern = 1.0 - (r_norm / ctx->interaction_radius);
            // Consider only particles closer than the interaction radius.
            if (kern > 0) {
              float half_D = (dt * dt) * (pressure * kern) / 2;
              vec2s half_d = glms_vec2_scale_as(r, half_D);
              ctx->ps[j].pos = glms_vec2_add(ctx->ps[j].pos, half_d);
              d = glms_vec2_sub(d, half_d);
            }
            }
      //     }
      //   }
      // }
      ctx->ps[i].pos = glms_vec2_sub(ctx->ps[i].pos, d);
    } // */

    // 8. Resolve collisions.
    // ...

    // 9. Compute next velocity.
    {
      ctx->ps[i].vel = glms_vec2_divs(
        glms_vec2_sub(ctx->ps[i].pos, old_pos),
        dt
      );
    }

    // 10. Contain particles in window bounds.
    {
      if (ctx->ps[i].pos.x < 0 || ctx->ps[i].pos.x >= ctx->window_width) {
        ctx->ps[i].vel.x *= -1.0f * ctx->collision_damping;
        ctx->ps[i].pos.x = SDL_clamp(ctx->ps[i].pos.x, 0, ctx->window_width - 1);
      }
      if (ctx->ps[i].pos.y < 0 || ctx->ps[i].pos.y >= ctx->window_height) {
        ctx->ps[i].vel.y *= -1.0f * ctx->collision_damping;
        ctx->ps[i].pos.y = SDL_clamp(ctx->ps[i].pos.y, 0, ctx->window_height - 1);
      }
    }
  }
}

