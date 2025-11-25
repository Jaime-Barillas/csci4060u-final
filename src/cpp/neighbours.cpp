#include "neighbours.h"

#include "particles.h"
#include "sim_opts.h"
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

Neighbours::Neighbours() { }

void Neighbours::cell_indexes(float px, float py, float pz, uint32_t grid_width, uint32_t &x, uint32_t &y, uint32_t &z) const {
  // NOTE: Cube shaped simulation area centered on origin.
  //       Need to offset `pos` since calculations rely on positive numbers.
  const float SIM_AREA_WIDTH = X_BOUNDS.y() - X_BOUNDS.x();
  const float SIM_AREA_HALF_WIDTH = X_BOUNDS.y();

  x = static_cast<uint32_t>((px + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
  y = static_cast<uint32_t>((py + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
  z = static_cast<uint32_t>((pz + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);

  // Handle boundary when x, y, or z are at their top bounds (ex. 1.0 for
  // sim bounds -1.0 to 1.0). When this happens, the above calculation will
  // result in index values of `grid_width`, but the valid range is
  // 0 to (grid_width - 1).
  x -= (x == grid_width);
  y -= (y == grid_width);
  z -= (z == grid_width);
}

uint32_t Neighbours::cell_index(float px, float py, float pz, uint32_t grid_width) const {
  uint32_t x, y, z;

  cell_indexes(px, py, pz, grid_width, x, y, z);

  return x + (grid_width * y) + (grid_width * grid_width * z);
}

void Neighbours::sort(Particles &input, Particles &output, uint32_t particle_count, uint32_t grid_width) {
  uint32_t bin_count = grid_width * grid_width * grid_width;

  for (auto &c : count_array) { c = 0; }

  for (size_t i = 0; i < particle_count; i++) {
    uint32_t j = cell_index(input.pos_x[i], input.pos_y[i], input.pos_z[i], grid_width);
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
    uint32_t j = cell_index(input.pos_x[i], input.pos_y[i], input.pos_z[i], grid_width);
    count_array[j] -= 1;
    output.pos_x[count_array[j]] = input.pos_x[i];
    output.pos_y[count_array[j]] = input.pos_y[i];
    output.pos_z[count_array[j]] = input.pos_z[i];
    output.vel_x[count_array[j]] = input.vel_x[i];
    output.vel_y[count_array[j]] = input.vel_y[i];
    output.vel_z[count_array[j]] = input.vel_z[i];
    output.forces_x[count_array[j]] = input.forces_x[i];
    output.forces_y[count_array[j]] = input.forces_y[i];
    output.forces_z[count_array[j]] = input.forces_z[i];
    output.density[count_array[j]] = input.density[i];
    output.pressure[count_array[j]] = input.pressure[i];
  }

  for (size_t i = 0; i < particle_count; i++) {
    input.pos_x[i] = output.pos_x[i];
    input.pos_y[i] = output.pos_y[i];
    input.pos_z[i] = output.pos_z[i];
    input.vel_x[i] = output.vel_x[i];
    input.vel_y[i] = output.vel_y[i];
    input.vel_z[i] = output.vel_z[i];
  }
}

void Neighbours::process(Particles &input, Particles &output, const SimOpts &opts) {
  uint32_t grid_width = std::floorf((X_BOUNDS.y() - X_BOUNDS.x()) / SUPPORT);
  uint32_t cell_count = grid_width * grid_width * grid_width;

  output.resize(opts.particle_count);
  count_array.resize(cell_count + 1);
  cell_starts.resize(cell_count + 1);

  sort(input, output, opts.particle_count, grid_width);
}

void Neighbours::neighbours_near(const Particles &ps, float px, float py, float pz, const SimOpts &opts, std::vector<std::pair<uint32_t, uint32_t>> &neighbours) {
  uint32_t grid_width = std::floorf((X_BOUNDS.y() - X_BOUNDS.x()) / opts.support);
  uint32_t x, y, z;
  cell_indexes(px, py, pz, grid_width, x, y, z);

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
        neighbours.push_back(std::pair<uint32_t, uint32_t>{start_idx, end_idx});
      }
    }
  }
}
