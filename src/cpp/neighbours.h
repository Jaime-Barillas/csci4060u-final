#pragma once

#include "particles.h"
#include "sim_opts.h"
#include <cstdint>
#include <libcommon/vec.h>
#include <vector>

class Neighbours {
  std::vector<uint32_t> count_array;
  std::vector<uint32_t> cell_starts;

  public:
    Neighbours();

    void cell_indexes(float px, float py, float pz, uint32_t grid_width, uint32_t &x, uint32_t &y, uint32_t &z) const;
    uint32_t cell_index(float px, float py, float pz, uint32_t grid_width) const;
    void sort(Particles &input, Particles &output, uint32_t particle_count, uint32_t grid_width);

    void process(Particles &input, Particles &output, const SimOpts &opts);
    void neighbours_near(const Particles &ps, float px, float py, float pz, const SimOpts &opts, std::vector<std::pair<uint32_t, uint32_t>> &neighbours);
};
