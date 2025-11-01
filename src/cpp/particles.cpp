#include "particles.h"
#include "util.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <ranges>
#include <vector>

namespace particles {
  Vec3 Vec3::normalized() {
    float length = std::sqrt((x * x) + (y * y) + (z * z));
    return { x / length, y / length, z / length};
  }

  void Vec3::negate() {
    x = -x;
    y = -y;
    z = -z;
  }

  void Vec3::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
  }

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
      particles.pos[i] = {
        .x = start + (x * step),
        .y = start + (y * step),
        .z = start + (z * step),
      };
      particles.vel[i] = {.x = 0, .y = 0, .z = 0};
      particles.pforce[i] = {.x = 0, .y = 0, .z = 0};
      particles.vforce[i] = {.x = 0, .y = 0, .z = 0};
      particles.eforce[i] = {.x = 0, .y = 0, .z = 0};
      particles.density[i] = 0;
      particles.pressure[i] = 0;
    }
  }

  template<>
  float kernel<PolyKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = 315.0f / (64 * std::numbers::pi_v<float> * util::pow(particles::SUPPORT, 9));

    float dx = particle.x - point.x;
    float dy = particle.y - point.y;
    float dz = particle.z - point.z;
    float distsqr = (dx * dx) + (dy * dy) + (dz * dz);
    float q = (particles::SUPPORT * particles::SUPPORT) - distsqr;

    // Check if within SUPPORT radius.
    q = (q < 0) ? 0 : q;
    return (q * q * q * COEFFICIENT);
  }

  template<>
  Vec3 kernel<SpikyGradKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = -45.0f / (std::numbers::pi_v<float> * util::pow(particles::SUPPORT, 6));

    float dx = particle.x - point.x;
    float dy = particle.y - point.y;
    float dz = particle.z - point.z;
    float dist = std::sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    float q = particles::SUPPORT - dist;

    if (q < 0 || dist <= 0) {
      return { 0, 0, 0 };
    }

    q = q * q * COEFFICIENT;

    return Vec3{
      .x = q * (dx / dist),
      .y = q * (dy / dist),
      .z = q * (dz / dist),
    };
  }

  template<>
  float kernel<ViscLaplKernel>(Vec3 &point, Vec3 &particle) {
    static constexpr float COEFFICIENT = 45.0f / (std::numbers::pi_v<float> * util::pow(particles::SUPPORT, 6));

    float dx = particle.x - point.x;
    float dy = particle.y - point.y;
    float dz = particle.z - point.z;
    float dist = std::sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    float q = particles::SUPPORT - dist;

    q = (q < 0) ? 0 : q;
    return q * COEFFICIENT;
  }

  void _cell_indexes(Vec3 pos, uint32_t grid_width, uint32_t &x, uint32_t &y, uint32_t &z) {
    // NOTE: Cube shaped simulation area centered on origin.
    //       Need to offset `pos` since calculations rely on positive numbers.
    const float SIM_AREA_WIDTH = particles::RIGHT_BOUND - particles::LEFT_BOUND;
    const float SIM_AREA_HALF_WIDTH = particles::RIGHT_BOUND;

    x = static_cast<uint32_t>((pos.x + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
    y = static_cast<uint32_t>((pos.y + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);
    z = static_cast<uint32_t>((pos.z + SIM_AREA_HALF_WIDTH) / SIM_AREA_WIDTH * grid_width);

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

      std::fflush(NULL);
    for (size_t i = 0; i < particle_count; i++) {
      uint32_t j = cell_index(ps.pos[i], grid_width);
      count[j] += 1;
    }

      std::fflush(NULL);
    for (size_t j = 1; j < (bin_count + 1); j++) {
      count[j] = count[j - 1] + count[j];
    }

      std::fflush(NULL);
    // Record start of bins
    bin_start[0] = 0;
    for (size_t j = 1; j < (bin_count + 1); j++) {
      bin_start[j] = count[j - 1];
    }

    for (int32_t i = (particle_count - 1); i > 0; i--) {
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

  void _fetch_neighbours(const Particles &ps, size_t p_idx, uint32_t grid_width, Particles &neighbours) {
    uint32_t x_index, y_index, z_index;
    _cell_indexes(ps.pos[p_idx], grid_width, x_index, y_index, z_index);

    neighbours.pos.clear();
    neighbours.vel.clear();
    neighbours.density.clear();
    neighbours.pressure.clear();

    for (int32_t i = x_index - 1; i < x_index + 1; i++) {
      if (i < 0 || i >= grid_width) continue;

      for (int32_t j = y_index - 1; j < y_index + 1; j++) {
        if (j < 0 || i >= grid_width) continue;

        for (int32_t k = z_index - 1; k < z_index + 1; k++) {
          if (k < 0 || i >= grid_width) continue;

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
    // static Particles neighbours;
    // static uint32_t grid_width = std::ceilf(2.0f / SUPPORT);
    size_t particle_count = ps.size();

    for (size_t i = 0; i < particle_count; i++) {
      // _fetch_neighbours(ps, i, grid_width, neighbours);
      // // if (neighbours.size() == 0) continue;

      ps.density[i] = 0.0;
      ps.pressure[i] = 0.0;

    // uint32_t x_index = static_cast<uint32_t>(std::floor((ps.pos[i].x + 1.0f) / 2.0f * 0.99 * grid_width));
    // uint32_t y_index = static_cast<uint32_t>(std::floor((ps.pos[i].y + 1.0f) / 2.0f * 0.99 * grid_width));
    // uint32_t z_index = static_cast<uint32_t>(std::floor((ps.pos[i].z + 1.0f) / 2.0f * 0.99 * grid_width));
    // uint32_t bin_idx = x_index + (y_index * grid_width) + (z_index * grid_width * grid_width);
    // std::printf("%4lu (%+.2f, %+.2f, %+.2f): (%u, %u, %u) %3u - Neighbour count: %3lu, bin start: %4u, bin end: %4u\n",
    //             i,
    //             ps.pos[i].x,
    //             ps.pos[i].y,
    //             ps.pos[i].z,
    //             x_index,
    //             y_index,
    //             z_index,
    //             bin_idx,
    //             neighbours.size(),
    //             bin_start[bin_idx],
    //             bin_start[bin_idx + 1]
    //           );
    // // if (neighbours.size() == 0) {
    // //   std::printf("Got 0 at: %lu\n", i);
    // // }

      for (size_t j = 0; j < particle_count; j++) {
        ps.density[i] += kernel<PolyKernel>(ps.pos[i], ps.pos[j]);
      }
      ps.pressure[i] = GAS_CONSTANT * (ps.density[i] - REST_DENSITY);
    }
    // std::fflush(NULL);
    // std::exit(0);
  }

  void calculate_pressure_forces(Particles &ps) {
    // FIXME: Something is wrong with the calculation.
    //        Particles tend to get 'sucked' into each other.
    //        Try smaller timesteps ?
    size_t particle_count = ps.size();

    Vec3 pressure_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 pressure_temp = { 0, 0, 0 };

      for (size_t j = 0; j < particle_count; j++) {
        pressure_kernel_temp = kernel<SpikyGradKernel>(ps.pos[i], ps.pos[j]);
        float pressure_factor = (ps.pressure[i] + ps.pressure[j]) / (2 * ps.density[j]);
        pressure_temp.x += pressure_kernel_temp.x * pressure_factor;
        pressure_temp.y += pressure_kernel_temp.y * pressure_factor;
        pressure_temp.z += pressure_kernel_temp.z * pressure_factor;
      }

      ps.pforce[i] = pressure_temp;
    }
  }

  void calculate_viscosity_forces(Particles &ps) {
    size_t particle_count = ps.size();
    
    float viscosity_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      particles::Vec3 viscosity_temp = { 0, 0, 0 };

      for (size_t j = 0; j < particle_count; j++) {
        viscosity_kernel_temp = kernel<ViscLaplKernel>(ps.pos[i], ps.pos[j]);
        float viscosity_factor_x = (ps.vel[j].x - ps.vel[i].x) / ps.density[j];
        float viscosity_factor_y = (ps.vel[j].y - ps.vel[i].y) / ps.density[j];
        float viscosity_factor_z = (ps.vel[j].z - ps.vel[i].z) / ps.density[j];
        viscosity_temp.x += VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_x;
        viscosity_temp.y += VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_y;
        viscosity_temp.z += VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_z;
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
      bool flow_up = ps.pos[i].y < 0
                   && std::abs(ps.pos[i].x) < FOUNTAIN_WIDTH
                   && std::abs(ps.pos[i].z) < FOUNTAIN_WIDTH;
      ps.eforce[i] = { 0, -GRAVITY_STRENGTH, 0 };
      if (flow_up) {
        ps.eforce[i].y = GRAVITY_STRENGTH * FOUNTAIN_STRENGTH;
      }
    }
  }

  void integrate(Particles &ps) {
    size_t particle_count = ps.size();

    particles::Vec3 acceleration;
    for (size_t i = 0; i < particle_count; i++) {
      // F = ma <=> a = F/m, m = 1.0 => a = F
      acceleration.x = ps.pforce[i].x + ps.vforce[i].x + ps.eforce[i].x;
      acceleration.y = ps.pforce[i].y + ps.vforce[i].y + ps.eforce[i].y;
      acceleration.z = ps.pforce[i].z + ps.vforce[i].z + ps.eforce[i].z;

      // v = a * dt;
      ps.vel[i].x += acceleration.x * (1.0f / 60); // FIXME: actually use delta time.
      ps.vel[i].y += acceleration.y * (1.0f / 60);
      ps.vel[i].z += acceleration.z * (1.0f / 60);

      // d = v * dt;
      ps.pos[i].x += ps.vel[i].x * (1.0f / 60);
      ps.pos[i].y += ps.vel[i].y * (1.0f / 60);
      ps.pos[i].z += ps.vel[i].z * (1.0f / 60);

      // Boundary conditions.
      if (ps.pos[i].x < LEFT_BOUND || ps.pos[i].x > RIGHT_BOUND) {
        ps.pos[i].x = std::clamp<float>(ps.pos[i].x, LEFT_BOUND, RIGHT_BOUND);
        ps.vel[i].x *= -0.5;
      }
      if (ps.pos[i].y < LOWER_BOUND || ps.pos[i].y > UPPER_BOUND) {
        ps.pos[i].y = std::clamp<float>(ps.pos[i].y, LOWER_BOUND, UPPER_BOUND);
        ps.vel[i].y *= -0.5;
      }
      if (ps.pos[i].z < BACKWARD_BOUND || ps.pos[i].z > FORWARD_BOUND) {
        ps.pos[i].z = std::clamp<float>(ps.pos[i].z, BACKWARD_BOUND, FORWARD_BOUND);
        ps.vel[i].z *= -0.5;
      }
    }
  }
}
