#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace particles {
  constexpr float USABLE_SPACE_MODIFIER = 0.8f;
  constexpr float PARTICLE_RADIUS = 0.075f;
  constexpr float SUPPORT = 0.33f;
  constexpr float GAS_CONSTANT = 0.3f;
  constexpr float REST_DENSITY = 300.0f;
  constexpr float VISCOSITY_CONSTANT = 0.1f;
  constexpr float GRAVITY_STRENGTH = 10.0f;
  constexpr float FOUNTAIN_WIDTH = 0.25;
  constexpr float FOUNTAIN_STRENGTH = 1.5;

  // Simulation area bounds.
  constexpr float LEFT_BOUND = -1.0;
  constexpr float RIGHT_BOUND = 1.0;
  constexpr float LOWER_BOUND = -1.0;
  constexpr float UPPER_BOUND = 1.0;
  constexpr float BACKWARD_BOUND = -1.0;
  constexpr float FORWARD_BOUND = 1.0;

  struct Vec3 {
    float x;
    float y;
    float z;

    Vec3 normalized();

    void negate();
    void operator*=(float scalar);
  };

  static_assert(std::is_standard_layout_v<Vec3>, "particles::Vec3 must have standard layout");

  struct Particles {
    std::vector<Vec3> pos;
    std::vector<Vec3> vel;
    std::vector<Vec3> pforce; // Pressure forces
    std::vector<Vec3> vforce; // Viscosity forces
    std::vector<Vec3> eforce; // External forces
    std::vector<float> density;
    std::vector<float> pressure;
  };

  void reset(
    Particles &particles,
    uint32_t count,
    float left_bound,
    float right_bound
  );

  // Neighbour Functions.
  void count_sort(Particles &ps);

  // Kernel Functions.
  struct PolyKernel      { using return_type = float; };
  struct SpikyGradKernel { using return_type = Vec3; };
  struct ViscLaplKernel  { using return_type = float; };

  template<typename T>
  concept Kernel = std::same_as<T, PolyKernel> ||
                   std::same_as<T, SpikyGradKernel> ||
                   std::same_as<T, ViscLaplKernel>;

  template<typename T>
  requires Kernel<T>
  typename T::return_type kernel(Vec3 &pos, Vec3 &particle);

  // Force Computation Functions.
  void calculate_density_pressure(Particles &ps);
  void calculate_pressure_forces(Particles &ps);
  void calculate_viscosity_forces(Particles &ps);
  void calculate_external_forces(Particles &ps);
  void integrate(Particles &ps);
}
