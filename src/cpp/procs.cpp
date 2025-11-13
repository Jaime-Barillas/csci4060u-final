#include "procs.h"

#include "sim_opts.h"
#include "util.h"
#include "neighbours.h"
#include <algorithm>

#include <cstdio>

namespace particles {
  template<>
  float kernel<PolyKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = 315.0f / (64 * std::numbers::pi_v<float> * util::pow(SUPPORT, 9));

    float distsqr = (particle - point).length_squared();
    float q = (SUPPORT * SUPPORT) - distsqr;

    // Check if within SUPPORT radius.
    q = (q < 0) ? 0 : q;
    return (q * q * q * COEFFICIENT);
  }

  template<>
  Vec3 kernel<SpikyGradKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = -45.0f / (std::numbers::pi_v<float> * util::pow(SUPPORT, 6));

    Vec3 difference = particle - point;
    float dist = difference.length();
    float q = SUPPORT - dist;

    if (q < 0 || dist <= 0) {
      return { 0, 0, 0 };
    }

    q = q * q * COEFFICIENT;

    // Manually normalizing (div by dist) avoids an extra sqrt().
    difference *= q * (1.0f / dist);
    return difference;
  }

  template<>
  float kernel<ViscLaplKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = 45.0f / (std::numbers::pi_v<float> * util::pow(SUPPORT, 6));

    float dist = (particle - point).length();
    float q = SUPPORT - dist;

    q = (q < 0) ? 0 : q;
    return q * COEFFICIENT;
  }

  void _cell_indexes(Vec3 pos, uint32_t grid_width, uint32_t &x, uint32_t &y, uint32_t &z) {
    // NOTE: Cube shaped simulation area centered on origin.
    //       Need to offset `pos` since calculations rely on positive numbers.
    const float SIM_AREA_WIDTH = RIGHT_BOUND - LEFT_BOUND;
    const float SIM_AREA_HALF_WIDTH = RIGHT_BOUND;

    x = static_cast<uint32_t>((pos.x() + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
    y = static_cast<uint32_t>((pos.y() + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
    z = static_cast<uint32_t>((pos.z() + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);

    // Handle boundary when x, y, or z are at their top bounds (e.x. 1.0 for
    // sim bounds -1.0 to 1.0). When this happens, the above calculation will
    // result in index values of `grid_width`, but the valid range is
    // 0 to (grid_width - 1).
    x -= (x == grid_width);
    y -= (y == grid_width);
    z -= (z == grid_width);
  }

  uint32_t cell_index(Vec3 pos, uint32_t grid_width) {
    uint32_t x_index, y_index, z_index;
    _cell_indexes(pos, grid_width, x_index, y_index, z_index);

    return x_index + (grid_width * y_index) + (grid_width * grid_width * z_index);
  }

  void _ensure_same_size(Particles &scratch, const Particles &ps) {
    scratch.resize(ps.size());
  }

  Particles scratch;
  std::vector<uint32_t> count;
  std::vector<uint32_t> bin_start;
  void count_sort(Particles &ps) {
    uint32_t grid_width = std::floorf(2.0f / SUPPORT);
    uint32_t bin_count = grid_width * grid_width * grid_width;
    size_t particle_count = ps.size();

    _ensure_same_size(scratch, ps);
    count.resize(bin_count + 1);
    bin_start.resize(bin_count + 1);
    for (auto &c : count) { c = 0; }

    for (size_t i = 0; i < particle_count; i++) {
      uint32_t j = cell_index(ps.pos[i], grid_width);
      count[j] += 1;
    }

    for (size_t j = 1; j < (bin_count + 1); j++) {
      count[j] = count[j - 1] + count[j];
    }

    // Record start of bins
    bin_start[0] = 0;
    for (size_t j = 1; j < (bin_count + 1); j++) {
      bin_start[j] = count[j - 1];
    }

    for (int32_t i = (particle_count - 1); i >= 0; i--) {
      uint32_t j = cell_index(ps.pos[i], grid_width);
      count[j] -= 1;
      scratch.pos[count[j]] = ps.pos[i];
      scratch.vel[count[j]] = ps.vel[i];
      scratch.pforce[count[j]] = ps.pforce[i];
      scratch.vforce[count[j]] = ps.vforce[i];
      scratch.eforce[count[j]] = ps.eforce[i];
      scratch.density[count[j]] = ps.density[i];
      scratch.pressure[count[j]] = ps.pressure[i];
    }

    for (size_t i = 0; i < particle_count; i++) {
      ps.pos[i] = scratch.pos[i];
      ps.vel[i] = scratch.vel[i];
      ps.pforce[i] = scratch.pforce[i];
      ps.vforce[i] = scratch.vforce[i];
      ps.eforce[i] = scratch.eforce[i];
      ps.density[i] = scratch.density[i];
      ps.pressure[i] = scratch.pressure[i];
    }
  }

  void fetch_neighbours(const Particles &ps, size_t p_idx, uint32_t grid_width, Particles &neighbours) {
    uint32_t x_index, y_index, z_index;
    _cell_indexes(ps.pos[p_idx], grid_width, x_index, y_index, z_index);

    neighbours.pos.clear();
    neighbours.vel.clear();
    neighbours.density.clear();
    neighbours.pressure.clear();

    int32_t x_start = static_cast<int32_t>(x_index) - 1;
    int32_t y_start = static_cast<int32_t>(y_index) - 1;
    int32_t z_start = static_cast<int32_t>(z_index) - 1;
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

    for (int32_t i = x_start; i <= x_end; i++) {
      for (int32_t j = y_start; j <= y_end; j++) {
        for (int32_t k = z_start; k <= z_end; k++) {
          uint32_t bin = i + (j * grid_width) + (k * grid_width * grid_width);
          uint32_t start_idx = bin_start[bin];
          uint32_t end_idx = bin_start[bin + 1];
          for (uint32_t p = start_idx; p < end_idx; p++) {
            neighbours.pos.push_back(ps.pos[p]);
            neighbours.vel.push_back(ps.vel[p]);
            neighbours.density.push_back(ps.density[p]);
            neighbours.pressure.push_back(ps.pressure[p]);
          }
        }
      }
    }
  }

  void calculate_density_pressure(Particles &ps, Neighbours &ns, const SimOpts &opts) {
    static Particles neighbours;
    // static Particles *neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);
    size_t particle_count = opts.particle_count;

    for (size_t i = 0; i < particle_count; i++) {
      ns.neighbours_near(ps, ps.pos[i], opts, neighbours);
      // fetch_neighbours(ps, i, grid_width, neighbours);

      ps.density[i] = 0.0;
      ps.pressure[i] = 0.0;

      for (size_t j = 0; j < neighbours.size(); j++) {
      // for (size_t j = 0; j < neighbours->size(); j++) {
        ps.density[i] += kernel<PolyKernel>(ps.pos[i], neighbours.pos[j]);
        // ps.density[i] += kernel<PolyKernel>(ps.pos[i], neighbours->pos[j]);
      }
      ps.pressure[i] = opts.gas_constant * (ps.density[i] - opts.rest_density);

      // ns.return_neighbour_list(neighbours);
    }
  }

  void calculate_pressure_forces(Particles &ps, Neighbours &ns, const SimOpts &opts) {
    // FIXME: Something is wrong with the calculation.
    //        Particles tend to get 'sucked' into each other.
    //        Try smaller timesteps ?
    // static Particles *neighbours;
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);
    size_t particle_count = ps.size();

    Vec3 pressure_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 pressure_temp{ 0, 0, 0 };
      ns.neighbours_near(ps, ps.pos[i], opts, neighbours);
      // fetch_neighbours(ps, i, grid_width, neighbours2);

      for (size_t j = 0; j < neighbours.size(); j++) {
      // for (size_t j = 0; j < neighbours->size(); j++) {
        pressure_kernel_temp = kernel<SpikyGradKernel>(ps.pos[i], neighbours.pos[j]);
        float pressure_factor = (ps.pressure[i] + neighbours.pressure[j]) / (2 * neighbours.density[j]);
        // pressure_kernel_temp = kernel<SpikyGradKernel>(ps.pos[i], neighbours->pos[j]);
        // float pressure_factor = (ps.pressure[i] + neighbours->pressure[j]) / (2 * neighbours->density[j]);
        pressure_kernel_temp *= pressure_factor;
        pressure_temp += pressure_kernel_temp;
      }
      ps.pforce[i] = pressure_temp;

      // ns.return_neighbour_list(neighbours);
    }
  }

  void calculate_viscosity_forces(Particles &ps, Neighbours &ns, const SimOpts &opts) {
    // static Particles *neighbours;
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);
    size_t particle_count = ps.size();
    
    float viscosity_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 viscosity_temp{ 0, 0, 0 };
      ns.neighbours_near(ps, ps.pos[i], opts, neighbours);
      // fetch_neighbours(ps, i, grid_width, neighbours);

      for (size_t j = 0; j < neighbours.size(); j++) {
      // for (size_t j = 0; j < neighbours->size(); j++) {
        viscosity_kernel_temp = kernel<ViscLaplKernel>(ps.pos[i], neighbours.pos[j]);
        Vec3 viscosity_factor = (neighbours.vel[j] - ps.vel[i]);
        viscosity_factor *= (1.0f / neighbours.density[j]);
        // viscosity_kernel_temp = kernel<ViscLaplKernel>(ps.pos[i], neighbours->pos[j]);
        // Vec3 viscosity_factor = (neighbours->vel[j] - ps.vel[i]);
        // viscosity_factor *= (1.0f / neighbours->density[j]);

        viscosity_factor *= opts.viscosity_constant * viscosity_kernel_temp;
        viscosity_temp += viscosity_factor;
      }
      ps.vforce[i] = viscosity_temp;

      // ns.return_neighbour_list(neighbours);
    }
  }

  void calculate_external_forces(Particles &ps) {
    size_t particle_count = ps.size();

    for (size_t i = 0; i < particle_count; i++) {
      /*
      ps.eforce[i] = ps.pos[i].normalized();
      ps.eforce[i].negate();
      ps.eforce[i] *= GRAVITY_STRENGTH;
      */
      bool flow_up = ps.pos[i].y() < 0
                   && std::abs(ps.pos[i].x()) < FOUNTAIN_WIDTH
                   && std::abs(ps.pos[i].z()) < FOUNTAIN_WIDTH;
      ps.eforce[i] = Vec3{ 0, -GRAVITY_STRENGTH, 0 };
      if (flow_up) {
        ps.eforce[i] = Vec3{ 0, GRAVITY_STRENGTH * FOUNTAIN_STRENGTH, 0 };
      }
    }
  }

  void integrate(Particles &ps) {
    size_t particle_count = ps.size();

    Vec3 acceleration;
    for (size_t i = 0; i < particle_count; i++) {
      // F = ma <=> a = F/m, m = 1.0 => a = F
      acceleration = ps.pforce[i] + ps.vforce[i] + ps.eforce[i];

      // v = a * dt;
      ps.vel[i] += acceleration * (1.0f / 60); // FIXME: actually use delta time.

      // d = v * dt;
      ps.pos[i] += ps.vel[i] * (1.0f / 60);

      // Boundary conditions.
      if (ps.pos[i].x() < LEFT_BOUND || ps.pos[i].x() > RIGHT_BOUND) {
        ps.pos[i].x(std::clamp<float>(ps.pos[i].x(), LEFT_BOUND, RIGHT_BOUND));
        ps.vel[i].x(ps.vel[i].x() * -0.5);
      }
      if (ps.pos[i].y() < LOWER_BOUND || ps.pos[i].y() > UPPER_BOUND) {
        ps.pos[i].y(std::clamp<float>(ps.pos[i].y(), LOWER_BOUND, UPPER_BOUND));
        ps.vel[i].y(ps.vel[i].y() * -0.5);
      }
      if (ps.pos[i].z() < BACKWARD_BOUND || ps.pos[i].z() > FORWARD_BOUND) {
        ps.pos[i].z(std::clamp<float>(ps.pos[i].z(), BACKWARD_BOUND, FORWARD_BOUND));
        ps.vel[i].z(ps.vel[i].z() * -0.5);
      }
    }
  }
}
