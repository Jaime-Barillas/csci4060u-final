#include "particles.h"
#include "util.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <ranges>
#include <vector>

namespace particles {
  void Particles::resize(size_t new_size) {
    pos.resize(new_size);
    vel.resize(new_size);
    pforce.resize(new_size);
    vforce.resize(new_size);
    eforce.resize(new_size);
    density.resize(new_size);
    pressure.resize(new_size);
  }

  size_t Particles::size() const { return pos.size(); }

  void reset(
    Particles &particles,
    uint32_t count,
    float left_bound,
    float right_bound
  ) {
    uint32_t length = (uint32_t)std::ceil(std::cbrt((float)count));
    float step = (right_bound - left_bound) * USABLE_SPACE_MODIFIER / length;
    float start = (-step * length) / 2.0f;

    particles.resize(count);

    for (uint32_t i : std::views::iota(0u, count)) {
      uint32_t x = i % length;
      uint32_t y = (i / length) % length;
      uint32_t z = i / (length * length);
      particles.pos[i] = Vec3{
        start + (x * step),
        start + (y * step),
        start + (z * step),
      };
      particles.vel[i] = Vec3{0, 0, 0};
      particles.pforce[i] = Vec3{0, 0, 0};
      particles.vforce[i] = Vec3{0, 0, 0};
      particles.eforce[i] = Vec3{0, 0, 0};
      particles.density[i] = 0;
      particles.pressure[i] = 0;
    }
  }

  template<>
  float kernel<PolyKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = 315.0f / (64 * std::numbers::pi_v<float> * util::pow(particles::SUPPORT, 9));

    float distsqr = (particle - point).length_squared();
    float q = (particles::SUPPORT * particles::SUPPORT) - distsqr;

    // Check if within SUPPORT radius.
    q = (q < 0) ? 0 : q;
    return (q * q * q * COEFFICIENT);
  }

  template<>
  Vec3 kernel<SpikyGradKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = -45.0f / (std::numbers::pi_v<float> * util::pow(particles::SUPPORT, 6));

    Vec3 difference = particle - point;
    float dist = difference.length();
    float q = particles::SUPPORT - dist;

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
    static constexpr float COEFFICIENT = 45.0f / (std::numbers::pi_v<float> * util::pow(particles::SUPPORT, 6));

    float dist = (particle - point).length();
    float q = particles::SUPPORT - dist;

    q = (q < 0) ? 0 : q;
    return q * COEFFICIENT;
  }

  void _cell_indexes(Vec3 pos, uint32_t grid_width, uint32_t &x, uint32_t &y, uint32_t &z) {
    // NOTE: Cube shaped simulation area centered on origin.
    //       Need to offset `pos` since calculations rely on positive numbers.
    const float SIM_AREA_WIDTH = particles::RIGHT_BOUND - particles::LEFT_BOUND;
    const float SIM_AREA_HALF_WIDTH = particles::RIGHT_BOUND;

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

    for (int32_t i = ((int32_t)x_index) - 1; i <= ((int32_t)x_index) + 1; i++) {
      if (i < 0 || i >= grid_width) continue;

      for (int32_t j = ((int32_t)y_index) - 1; j <= ((int32_t)y_index) + 1; j++) {
        if (j < 0 || j >= grid_width) continue;

        for (int32_t k = ((int32_t)z_index) - 1; k <= ((int32_t)z_index) + 1; k++) {
          if (k < 0 || k >= grid_width) continue;

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

  void calculate_density_pressure(Particles &ps) {
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / SUPPORT);
    size_t particle_count = ps.size();

    for (size_t i = 0; i < particle_count; i++) {
      fetch_neighbours(ps, i, grid_width, neighbours);

      ps.density[i] = 0.0;
      ps.pressure[i] = 0.0;

      for (size_t j = 0; j < neighbours.size(); j++) {
        ps.density[i] += kernel<PolyKernel>(ps.pos[i], neighbours.pos[j]);
      }
      ps.pressure[i] = GAS_CONSTANT * (ps.density[i] - REST_DENSITY);
    }
  }

  void calculate_pressure_forces(Particles &ps) {
    // FIXME: Something is wrong with the calculation.
    //        Particles tend to get 'sucked' into each other.
    //        Try smaller timesteps ?
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / SUPPORT);
    size_t particle_count = ps.size();

    Vec3 pressure_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 pressure_temp{ 0, 0, 0 };
      fetch_neighbours(ps, i, grid_width, neighbours);

      for (size_t j = 0; j < neighbours.size(); j++) {
        pressure_kernel_temp = kernel<SpikyGradKernel>(ps.pos[i], neighbours.pos[j]);
        float pressure_factor = (ps.pressure[i] + neighbours.pressure[j]) / (2 * neighbours.density[j]);
        pressure_kernel_temp *= pressure_factor;
        pressure_temp += pressure_kernel_temp;
      }

      ps.pforce[i] = pressure_temp;
    }
  }

  void calculate_viscosity_forces(Particles &ps) {
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / SUPPORT);
    size_t particle_count = ps.size();
    
    float viscosity_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 viscosity_temp{ 0, 0, 0 };
      fetch_neighbours(ps, i, grid_width, neighbours);

      for (size_t j = 0; j < neighbours.size(); j++) {
        viscosity_kernel_temp = kernel<ViscLaplKernel>(ps.pos[i], neighbours.pos[j]);
        Vec3 viscosity_factor = (neighbours.vel[j] - ps.vel[i]);
        viscosity_factor *= (1.0f / neighbours.density[j]);
        viscosity_factor *= VISCOSITY_CONSTANT * viscosity_kernel_temp;
        viscosity_temp += viscosity_factor;
      }

      ps.vforce[i] = viscosity_temp;
    }
  }

  void calculate_external_forces(Particles &ps) {
    size_t particle_count = ps.size();

    for (size_t i = 0; i < particle_count; i++) {
      /*
      ps.eforce[i] = ps.pos[i].normalized();
      ps.eforce[i].negate();
      ps.eforce[i] *= particles::GRAVITY_STRENGTH;
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
