#include "neighbours.h"

#include "particles.h"
#include "sim_opts.h"
#include <cmath>
#include <cstdint>

Neighbours::Neighbours() { }

void Neighbours::cell_indexes(Vec3 pos, uint32_t grid_width, uint32_t &x, uint32_t &y, uint32_t &z) const {
  // NOTE: Cube shaped simulation area centered on origin.
  //       Need to offset `pos` since calculations rely on positive numbers.
  const float SIM_AREA_WIDTH = X_BOUNDS.y() - X_BOUNDS.x();
  const float SIM_AREA_HALF_WIDTH = X_BOUNDS.y();

  x = static_cast<uint32_t>((pos.x() + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
  y = static_cast<uint32_t>((pos.y() + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
  z = static_cast<uint32_t>((pos.z() + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);

  // Handle boundary when x, y, or z are at their top bounds (ex. 1.0 for
  // sim bounds -1.0 to 1.0). When this happens, the above calculation will
  // result in index values of `grid_width`, but the valid range is
  // 0 to (grid_width - 1).
  x -= (x == grid_width);
  y -= (y == grid_width);
  z -= (z == grid_width);
}

uint32_t Neighbours::cell_index(Vec3 pos, uint32_t grid_width) const {
  uint32_t x, y, z;

  cell_indexes(pos, grid_width, x, y, z);

  return x + (grid_width * y) + (grid_width * grid_width * z);
}

void Neighbours::sort(Particles &ps, uint32_t particle_count, uint32_t grid_width) {
  uint32_t bin_count = grid_width * grid_width * grid_width;

  for (auto &c : count_array) { c = 0; }

  for (size_t i = 0; i < particle_count; i++) {
    uint32_t j = cell_index(ps.pos[i], grid_width);
    count_array[j] += 1;
  }

  for (size_t j = 1; j < (bin_count + 1); j++) {
    count_array[j] = count_array[j - 1] + count_array[j];
  }

  // Record start of bins
  cell_starts[0] = 0;
  for (size_t j = 1; j < (bin_count + 1); j++) {
    cell_starts[j] = count_array[j - 1];
  }

  for (int32_t i = (particle_count - 1); i >= 0; i--) {
    uint32_t j = cell_index(ps.pos[i], grid_width);
    count_array[j] -= 1;
    sorted.pos[count_array[j]] = ps.pos[i];
    sorted.vel[count_array[j]] = ps.vel[i];
    sorted.pforce[count_array[j]] = ps.pforce[i];
    sorted.vforce[count_array[j]] = ps.vforce[i];
    sorted.eforce[count_array[j]] = ps.eforce[i];
    sorted.density[count_array[j]] = ps.density[i];
    sorted.pressure[count_array[j]] = ps.pressure[i];
  }

  for (size_t i = 0; i < particle_count; i++) {
    ps.pos[i] = sorted.pos[i];
    ps.vel[i] = sorted.vel[i];
    ps.pforce[i] = sorted.pforce[i];
    ps.vforce[i] = sorted.vforce[i];
    ps.eforce[i] = sorted.eforce[i];
    ps.density[i] = sorted.density[i];
    ps.pressure[i] = sorted.pressure[i];
  }
}

void Neighbours::process(Particles &ps, const SimOpts & opts) {
  uint32_t grid_width = std::floorf((X_BOUNDS.y() - X_BOUNDS.x()) / SUPPORT);
  uint32_t cell_count = grid_width * grid_width * grid_width;

  sorted.resize(opts.particle_count);
  count_array.resize(cell_count + 1);
  cell_starts.resize(cell_count + 1);

  sort(ps, opts.particle_count, grid_width);
}

void Neighbours::neighbours_near(const Particles &ps, Vec3 pos, const SimOpts &opts, Particles &neighbours) {
  uint32_t grid_width = std::floorf((X_BOUNDS.y() - X_BOUNDS.x()) / opts.support);
  uint32_t x, y, z;
  cell_indexes(pos, grid_width, x, y, z);

  int32_t x_start = static_cast<int32_t>(x) - 1;
  int32_t y_start = static_cast<int32_t>(y) - 1;
  int32_t z_start = static_cast<int32_t>(z) - 1;
  int32_t x_end = x_start + 2;
  int32_t y_end = y_start + 2;
  int32_t z_end = z_start + 2;

  // If the cell coordinates of the particle in question are at either extreme,
  // (e.g. (0, 0, 0) or (grid_width, grid_width, grid_width)), the above
  // offsets will push the starting index out of bounds.
  x_start += (x_start == -1);
  y_start += (y_start == -1);
  z_start += (z_start == -1);
  x_end -= (x_end == grid_width);
  y_end -= (y_end == grid_width);
  z_end -= (z_end == grid_width);

  neighbours.clear();

  for (int32_t k = z_start; k <= z_end; k++) {
    for (int32_t j = y_start; j <= y_end; j++) {
      for (int32_t i = x_start; i <= x_end; i++) {
        uint32_t cell = i + (j * grid_width) + (k * grid_width * grid_width);
        uint32_t start_idx = cell_starts[cell];
        uint32_t end_idx = cell_starts[cell + 1];
        neighbours.pos.insert(neighbours.pos.end(), ps.pos.begin() + start_idx, ps.pos.begin() + end_idx);
        neighbours.vel.insert(neighbours.vel.end(), ps.vel.begin() + start_idx, ps.vel.begin() + end_idx);
        neighbours.density.insert(neighbours.density.end(), ps.density.begin() + start_idx, ps.density.begin() + end_idx);
        neighbours.pressure.insert(neighbours.pressure.end(), ps.pressure.begin() + start_idx, ps.pressure.begin() + end_idx);
      }
    }
  }
}
