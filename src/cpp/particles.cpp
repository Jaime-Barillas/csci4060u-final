#include "particles.h"
#include <cmath>
#include <cstdint>
#include <ranges>
#include <ui.h>
#include <vector>

namespace Particles {
  void reset(
    std::vector<Particle> &particles,
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
      uint32_t y = i / length;
      uint32_t z = i / (length * length);
      particles[i].pos = {
        .x = start + (x * step),
        .y = start + (y * step),
        .z = start + (z * step),
      };
      particles[i].vel = {.x = 0, .y = 0, .z = 0};
    }
  }
}
