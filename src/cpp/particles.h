#pragma once

#include <cstdint>
#include <ui.h>
#include <vector>

template <typename T>
static consteval T cube(T n) { return n * n * n; }

namespace Particles {
  constexpr int32_t DEFAULT_PARTICLE_COUNT = cube(100);
  constexpr float USABLE_SPACE_MODIFIER = 0.8f;

  void reset(
    std::vector<Particle> &particles,
    uint32_t count,
    float left_bound,
    float right_bound
  );
}
