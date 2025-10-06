#pragma once

#include <concepts>
#include <cstdint>
#include <vector>

namespace particles {
  constexpr float USABLE_SPACE_MODIFIER = 0.8f;
  constexpr float SUPPORT = 10;

  struct Vec3 {
    float x;
    float y;
    float z;
  };

  struct Particle {
    Vec3 pos;
    Vec3 vel;
  };

  void reset(
    std::vector<Particle> &particles,
    uint32_t count,
    float left_bound,
    float right_bound
  );

  struct PolyKernel      { using return_type = float; };
  struct SpikyGradKernel { using return_type = float; };
  struct ViscLaplKernel  { using return_type = float; };

  template<typename T>
  concept Kernel = std::same_as<T, PolyKernel> ||
                   std::same_as<T, SpikyGradKernel> ||
                   std::same_as<T, ViscLaplKernel>;

  template<typename T>
  requires Kernel<T>
  typename T::return_type kernel(Vec3 &pos, Vec3 &particle);
}
