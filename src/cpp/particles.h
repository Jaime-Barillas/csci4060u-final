#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

namespace particles {
  constexpr float USABLE_SPACE_MODIFIER = 0.8f;
  constexpr float SUPPORT = 0.33f;
  constexpr float GAS_CONSTANT = 1.0f;
  constexpr float REST_DENSITY = 125.0f;
  constexpr float VISCOSITY_CONSTANT = 0.0f;

  struct Vec3 {
    float x;
    float y;
    float z;
  };

  struct Particle {
    Vec3 pos;
    Vec3 vel;
    Vec3 pforce; // Pressure forces
    Vec3 vforce; // Viscosity forces
    Vec3 eforce; // External forces
    float density;
    float pressure;
  };

  void reset(
    std::vector<Particle> &particles,
    uint32_t count,
    float left_bound,
    float right_bound
  );

  struct PolyKernel      { using return_type = float; };
  struct SpikyGradKernel { using return_type = Vec3; };
  struct ViscLaplKernel  { using return_type = Vec3; };

  template<typename T>
  concept Kernel = std::same_as<T, PolyKernel> ||
                   std::same_as<T, SpikyGradKernel> ||
                   std::same_as<T, ViscLaplKernel>;

  template<typename T>
  requires Kernel<T>
  typename T::return_type kernel(Vec3 &pos, Vec3 &particle);
}
