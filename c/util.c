#include <string.h> // memset().
#include <SDL3/SDL_stdinc.h>
#include "sim.h"
#include "util.h"

int64_t cell_for_pos(int64_t x, int64_t y, const SimCtx *ctx) {
    return ((ctx->grid_width * (y / ctx->cell_size)) + (x / ctx->cell_size));
}

void sort_by_cell(SimCtx *ctx) {
    // Counting + Cycle sort. Sorts particles based on the grid cell (bin) they
    // belong to using its index as their key.
    memset(ctx->cell_pcount, 0, ctx->cell_count * sizeof(int64_t));

    for (int64_t i = 0; i < ctx->pcount; i++) {
        int64_t x = (int64_t)ctx->ps[i].pos.x;
        int64_t y = (int64_t)ctx->ps[i].pos.y;
        int64_t key = cell_for_pos(x, y, ctx);
        ctx->cell_pcount[key] += 1;
    }

    // Keep 0's in empty bins even when calculating the prefix sum.
    // These 0's will be later used to detect empty bins during the simulation
    // steps.
    int64_t last_non_zero = 0;
    for (int64_t i = 1; i < ctx->cell_count; i++) {
        if (ctx->cell_pcount[i] != 0) {
            ctx->cell_pcount[i] += ctx->cell_pcount[last_non_zero];
            last_non_zero = i;
        }
    }

    for (int64_t i = ctx->pcount -1; i >= 0; i--) {
        int64_t j = cell_for_pos(ctx->ps[i].pos.x, ctx->ps[i].pos.y, ctx);
        ctx->cell_pcount[j] -= 1;
        ctx->ps2[ctx->cell_pcount[j]] = ctx->ps[i];
    }
    Particle *temp = ctx->ps;
    ctx->ps = ctx->ps2;
    ctx->ps2 = temp;
    // // In-place cycle sort.
    // // See: https://stackoverflow.com/a/15719967
    // // See: https://stackoverflow.com/a/62598539
    // Particle temp;
    // for (int64_t i = 0; i < ctx->pcount; i++) {
    //     int64_t x = (int64_t)ctx->ps[i].pos.x;
    //     int64_t y = (int64_t)ctx->ps[i].pos.y;
    //     int64_t key = cell_for_pos(x, y, ctx);
    //     int64_t ncount = ctx->cell_pcount[key] - 1;
    //     while (i < ncount) {
    //         temp = ctx->ps[ncount];
    //         ctx->ps[ncount] = ctx->ps[i];
    //         ctx->ps[i] = temp;
    //         ctx->cell_pcount[key] = ncount;
    //         ncount -= 1;
    //     }
    // }
}

