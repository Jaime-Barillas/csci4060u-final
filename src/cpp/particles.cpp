#include "particles.h"
#include "util.h"
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

  void reset(
    Particles &particles,
    uint32_t count,
    float left_bound,
    float right_bound
  ) {
    uint32_t length = (uint32_t)std::ceil(std::cbrt((float)count));
    float step = (right_bound - left_bound) * USABLE_SPACE_MODIFIER / length;
    float start = (-step * length) / 2.0f;

    particles.pos.resize(count);
    particles.vel.resize(count);
    particles.pforce.resize(count);
    particles.vforce.resize(count);
    particles.eforce.resize(count);
    particles.density.resize(count);
    particles.pressure.resize(count);

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
}
