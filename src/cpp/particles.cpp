#include "particles.h"
#include <cmath>
#include <cstdint>
#include <numbers>
#include <ranges>
#include <ui.h>
#include "util.h"
#include <vector>

namespace particles {
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
  float kernel<SpikyKernel>(Vec3 &point, Vec3 &particle) {
    // TODO:
    return 2;
  }
}
