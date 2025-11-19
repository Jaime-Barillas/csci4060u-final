#include "procs.h"

#include "sim_opts.h"
#include "util.h"
#include "neighbours.h"
#include <algorithm>

#include <cstdio>

namespace particles {
  /*** Kernels ***/
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

  /*** Force Calculations ***/
  void calculate_density_pressure(Particles &ps, Neighbours &ns, const SimOpts &opts) {
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);
    size_t particle_count = opts.particle_count;

    for (size_t i = 0; i < particle_count; i++) {
      ns.neighbours_near(ps, ps.pos[i], opts, neighbours);

      ps.density[i] = 0.0;
      ps.pressure[i] = 0.0;

      for (size_t j = 0; j < neighbours.size(); j++) {
        ps.density[i] += kernel<PolyKernel>(ps.pos[i], neighbours.pos[j]);
      }
      ps.pressure[i] = opts.gas_constant * (ps.density[i] - opts.rest_density);
    }
  }

  void calculate_pressure_forces(Particles &ps, Neighbours &ns, const SimOpts &opts) {
    // FIXME: Something is wrong with the calculation.
    //        Particles tend to get 'sucked' into each other.
    //        Try smaller timesteps ?
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);
    size_t particle_count = ps.size();

    Vec3 pressure_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 pressure_temp{ 0, 0, 0 };
      ns.neighbours_near(ps, ps.pos[i], opts, neighbours);

      for (size_t j = 0; j < neighbours.size(); j++) {
        pressure_kernel_temp = kernel<SpikyGradKernel>(ps.pos[i], neighbours.pos[j]);
        float pressure_factor = (ps.pressure[i] + neighbours.pressure[j]) / (2 * neighbours.density[j]);
        pressure_kernel_temp *= pressure_factor;
        pressure_temp += pressure_kernel_temp;
      }
      ps.pforce[i] = pressure_temp;
    }
  }

  void calculate_viscosity_forces(Particles &ps, Neighbours &ns, const SimOpts &opts) {
    static Particles neighbours;
    static uint32_t grid_width = std::floorf(2.0f / opts.support);
    size_t particle_count = ps.size();
    
    float viscosity_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 viscosity_temp{ 0, 0, 0 };
      ns.neighbours_near(ps, ps.pos[i], opts, neighbours);

      for (size_t j = 0; j < neighbours.size(); j++) {
        viscosity_kernel_temp = kernel<ViscLaplKernel>(ps.pos[i], neighbours.pos[j]);
        Vec3 viscosity_factor = (neighbours.vel[j] - ps.vel[i]);
        viscosity_factor *= (1.0f / neighbours.density[j]);

        viscosity_factor *= opts.viscosity_constant * viscosity_kernel_temp;
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
