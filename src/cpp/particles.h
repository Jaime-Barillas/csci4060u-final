#pragma once

#include <concepts>
#include <cstdint>
#include <ui.h>
#include "util.h"
#include <vector>

namespace particles {
  constexpr int32_t DEFAULT_PARTICLE_COUNT = util::pow(10, 3);
  constexpr float USABLE_SPACE_MODIFIER = 0.8f;
  constexpr float SUPPORT = 10;

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
