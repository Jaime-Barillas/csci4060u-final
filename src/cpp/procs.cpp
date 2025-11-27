#include "procs.h"

#include "sim_opts.h"
#include "util.h"
#include "neighbours.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace particles {
  /*** Helper Functions ***/
  static inline float distance_squared(
    float x_1,
    float y_1,
    float z_1,
    float x_2,
    float y_2,
    float z_2
  ) {
    float dist_x = x_2 - x_1;
    float dist_y = y_2 - y_1;
    float dist_z = z_2 - z_1;
    return (dist_x * dist_x) + (dist_y * dist_y) + (dist_z * dist_z);
  }

  /*** Kernels ***/
  template<>
  float kernel<PolyKernel>(
    float point_x,
    float point_y,
    float point_z,
    float particle_x,
    float particle_y,
    float particle_z
  ) {
    static constexpr float COEFFICIENT = 315.0f / (64 * std::numbers::pi_v<float> * util::pow(SUPPORT, 9));

    float distsqr = distance_squared(point_x, point_y, point_z, particle_x, particle_y, particle_z);
    float q = (SUPPORT * SUPPORT) - distsqr;

    // Check if within SUPPORT radius.
    q = (q < 0) ? 0 : q;
    return (q * q * q * COEFFICIENT);
  }

  template<>
  Vec3 kernel<SpikyGradKernel>(
    float point_x,
    float point_y,
    float point_z,
    float particle_x,
    float particle_y,
    float particle_z
  ) {
    static constexpr float COEFFICIENT = -45.0f / (std::numbers::pi_v<float> * util::pow(SUPPORT, 6));

    float dist_x = particle_x - point_x;
    float dist_y = particle_y - point_y;
    float dist_z = particle_z - point_z;
    float distsqr = (dist_x * dist_x) + (dist_y * dist_y) + (dist_z * dist_z);
    float dist = std::sqrtf(distsqr);
    float q = SUPPORT - dist;

    if (q < 0 || dist <= 0) {
      return { 0, 0, 0 };
    }

    q = q * q * COEFFICIENT;

    // Manually normalizing (div by dist) avoids an extra sqrt().
    float factor = q * (1.0f / dist);
    Vec3 difference{dist_x * factor, dist_y * factor, dist_z * factor};
    return difference;
  }

  template<>
  float kernel<ViscLaplKernel>(
    float point_x,
    float point_y,
    float point_z,
    float particle_x,
    float particle_y,
    float particle_z
  ) {
    static constexpr float COEFFICIENT = 45.0f / (std::numbers::pi_v<float> * util::pow(SUPPORT, 6));

    float distsqr = distance_squared(point_x, point_y, point_z, particle_x, particle_y, particle_z);
    float dist = std::sqrtf(distsqr);
    float q = SUPPORT - dist;

    q = (q < 0) ? 0 : q;
    return q * COEFFICIENT;
  }

  /*** Force Calculations ***/
  void calculate_density_pressure(Particles &input, Particles &output, Neighbours &ns, size_t pstart, size_t pend, const SimOpts &opts) {
    thread_local std::vector<std::pair<uint32_t, uint32_t>> neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);

    for (size_t i = pstart; i < pend; i++) {
      ns.neighbours_near(input, input.pos_x[i], input.pos_y[i], input.pos_z[i], opts, neighbours);
      size_t neighbour_count = neighbours.size();

      output.density[i] = 0.0;
      output.pressure[i] = 0.0;

      for (size_t pair_idx = 0; pair_idx < neighbour_count; pair_idx++) {
        auto [start, end] = neighbours[pair_idx];
        for (size_t j = start; j < end; j++) {
          output.density[i] += kernel<PolyKernel>(input.pos_x[i], input.pos_y[i], input.pos_z[i], input.pos_x[j], input.pos_y[j], input.pos_z[j]);
        }
      }
      output.pressure[i] = opts.gas_constant * (output.density[i] - opts.rest_density);
    }
  }

  void calculate_pressure_forces(Particles &input, Particles &output, Neighbours &ns, size_t pstart, size_t pend, const SimOpts &opts) {
    // FIXME: Something is wrong with the calculation.
    //        Particles tend to get 'sucked' into each other.
    //        Try smaller timesteps ?
    thread_local std::vector<std::pair<uint32_t, uint32_t>> neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);

    Vec3 pressure_kernel_temp;
    for (size_t i = pstart; i < pend; i++) {
      Vec3 pressure_temp{ 0, 0, 0 };
      ns.neighbours_near(input, input.pos_x[i], input.pos_y[i], input.pos_z[i], opts, neighbours);

      for (size_t pair_idx = 0; pair_idx < neighbours.size(); pair_idx++) {
        auto [start, end] = neighbours[pair_idx];
        for (size_t j = start; j < end; j++) {
          pressure_kernel_temp = kernel<SpikyGradKernel>(input.pos_x[i], input.pos_y[i], input.pos_z[i], input.pos_x[j], input.pos_y[j], input.pos_z[j]);
          float pressure_factor = (input.pressure[i] + input.pressure[j]) / (2 * input.density[j]);
          pressure_kernel_temp *= pressure_factor;
          pressure_temp += pressure_kernel_temp;
        }
      }
      output.forces_x[i] = pressure_temp.x();
      output.forces_y[i] = pressure_temp.y();
      output.forces_z[i] = pressure_temp.z();
    }
  }

  void calculate_viscosity_forces(Particles &input, Particles &output, Neighbours &ns, size_t pstart, size_t pend, const SimOpts &opts) {
    thread_local std::vector<std::pair<uint32_t, uint32_t>> neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);
    
    float viscosity_kernel_temp;
    for (size_t i = pstart; i < pend; i++) {
      Vec3 viscosity_temp{ 0, 0, 0 };
      ns.neighbours_near(input, input.pos_x[i], input.pos_y[i], input.pos_z[i], opts, neighbours);

      for (size_t pair_idx = 0; pair_idx < neighbours.size(); pair_idx++) {
        auto [start, end] = neighbours[pair_idx];
        for (size_t j = start; j < end; j++) {
          viscosity_kernel_temp = kernel<ViscLaplKernel>(input.pos_x[i], input.pos_y[i], input.pos_z[i], input.pos_x[j], input.pos_y[j], input.pos_z[j]);
          Vec3 viscosity_factor = {
            input.vel_x[j] - input.vel_x[i],
            input.vel_y[j] - input.vel_y[i],
            input.vel_z[j] - input.vel_z[i]
          };
          viscosity_factor *= (1.0f / input.density[j]);

          viscosity_factor *= opts.viscosity_constant * viscosity_kernel_temp;
          viscosity_temp += viscosity_factor;
        }
      }
      output.forces_x[i] += viscosity_temp.x();
      output.forces_y[i] += viscosity_temp.y();
      output.forces_z[i] += viscosity_temp.z();
    }
  }

  void calculate_external_forces(Particles &input, Particles &output, size_t pstart, size_t pend) {
    float force_temp_x = 0;
    float force_temp_y = 0;
    float force_temp_z = 0;
    for (size_t i = pstart; i < pend; i++) {
      bool flow_up = input.pos_y[i] < 0
                   && std::abs(input.pos_x[i]) < FOUNTAIN_WIDTH
                   && std::abs(input.pos_z[i]) < FOUNTAIN_WIDTH;
      force_temp_y = -GRAVITY_STRENGTH;
      if (flow_up) {
        force_temp_y = GRAVITY_STRENGTH * FOUNTAIN_STRENGTH;
      }
      output.forces_x[i] += force_temp_x;
      output.forces_y[i] += force_temp_y;
      output.forces_z[i] += force_temp_z;
    }
  }

  void integrate(Particles &output, size_t pstart, size_t pend) {
    float accel_x;
    float accel_y;
    float accel_z;

    for (size_t i = pstart; i < pend; i++) {
      // F = ma <=> a = F/m, m = 1.0 => a = F
      accel_x = output.forces_x[i];
      accel_y = output.forces_y[i];
      accel_z = output.forces_z[i];

      // v = a * dt;
      output.vel_x[i] += accel_x * (1.0f / 60); // FIXME: actually use delta time.
      output.vel_y[i] += accel_y * (1.0f / 60);
      output.vel_z[i] += accel_z * (1.0f / 60);

      // d = v * dt;
      output.pos_x[i] += output.vel_x[i] * (1.0f / 60);
      output.pos_y[i] += output.vel_y[i] * (1.0f / 60);
      output.pos_z[i] += output.vel_z[i] * (1.0f / 60);

      // Boundary conditions.
      if (output.pos_x[i] < LEFT_BOUND || output.pos_x[i] > RIGHT_BOUND) {
        output.pos_x[i] = std::clamp<float>(output.pos_x[i], LEFT_BOUND, RIGHT_BOUND);
        output.vel_x[i] = output.vel_x[i] * -0.5;
      }
      if (output.pos_y[i] < LOWER_BOUND || output.pos_y[i] > UPPER_BOUND) {
        output.pos_y[i] = std::clamp<float>(output.pos_y[i], LOWER_BOUND, UPPER_BOUND);
        output.vel_y[i] = output.vel_y[i] * -0.5;
      }
      if (output.pos_z[i] < BACKWARD_BOUND || output.pos_z[i] > FORWARD_BOUND) {
        output.pos_z[i] = std::clamp<float>(output.pos_z[i], BACKWARD_BOUND, FORWARD_BOUND);
        output.vel_z[i] = output.vel_z[i] * -0.5;
      }
    }
  }
}
