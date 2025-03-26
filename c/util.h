#ifndef UTIL_H
#define UTIL_H

#include "sim.h"

int64_t cell_for_pos(int64_t x, int64_t y, const SimCtx *ctx);
void sort_by_cell(SimCtx *ctx);

#endif
