#pragma once

#include "particles.h"
#include "sim_opts.h"
#include <cstdint>
#include <libcommon/vec.h>
#include <vector>

class Neighbours {
  Particles sorted;
  std::vector<uint32_t> count_array;
  std::vector<uint32_t> cell_starts;

  std::vector<Particles*> pool;
  std::vector<Particles*>in_use;

  public:
    Neighbours();

  void cell_indexes(Vec3 pos, uint32_t grid_width, uint32_t &x, uint32_t &y, uint32_t &z) const;
  uint32_t cell_index(Vec3 pos, uint32_t grid_width) const;
  void sort(Particles &ps, uint32_t particle_count, uint32_t grid_width);
  Particles *get_free_neighbour_list();

    void process(Particles &ps, const SimOpts &opts);
    void neighbours_near(const Particles &ps, Vec3 pos, const SimOpts &opts, Particles &neighbours);
    void return_neighbour_list(Particles *neighbours);
};
