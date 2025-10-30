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

  uint32_t _bin(Vec3 pos, uint32_t grid_width) {
    // Offset position by (1, 1, 1) since loc
    uint32_t x_index = (pos.x + 1.0f) / grid_width;
    uint32_t y_index = (pos.y + 1.0f) / grid_width;
    uint32_t z_index = (pos.z + 1.0f) / grid_width;

    return x_index + (grid_width * y_index) + (grid_width * grid_width * z_index);
  }

  void _ensure_same_size(Particles &scratch, const Particles &ps) {
    scratch.pos.resize(ps.pos.size());
    scratch.vel.resize(ps.vel.size());
    scratch.pforce.resize(ps.pforce.size());
    scratch.vforce.resize(ps.vforce.size());
    scratch.eforce.resize(ps.eforce.size());
    scratch.density.resize(ps.density.size());
    scratch.pressure.resize(ps.pressure.size());
  }

  Particles scratch;
  std::vector<uint32_t> count;
  void count_sort(Particles &ps) {
    uint32_t grid_width = std::ceilf(2.0f / SUPPORT);
    uint32_t bin_count = grid_width * grid_width * grid_width;
    size_t particle_count = ps.pos.size();

    _ensure_same_size(scratch, ps);
    count.resize(bin_count + 1);
    for (auto &c : count) { c = 0; }

    for (size_t i = 0; i < particle_count; i++) {
      uint32_t j = _bin(ps.pos[i], grid_width);
      count[j] += 1;
    }

    for (size_t j = 1; j < (bin_count + 1); j++) {
      count[j] = count[j - 1] + count[j];
    }

    for (int32_t i = (particle_count - 1); i > 0; i--) {
      uint32_t j = _bin(ps.pos[i], grid_width);
      count[j] -= 1;
      scratch.pos[count[j]] = ps.pos[i];
      scratch.vel[count[j]] = ps.vel[i];
      scratch.pforce[count[j]] = ps.pforce[i];
      scratch.vforce[count[j]] = ps.vforce[i];
      scratch.eforce[count[j]] = ps.eforce[i];
      scratch.density[count[j]] = ps.density[i];
      scratch.pressure[count[j]] = ps.pressure[i];
    }

    for (size_t i = 0; i < particle_count; i++) {
      ps.pos[i] = scratch.pos[i];
      ps.vel[i] = scratch.vel[i];
      ps.pforce[i] = scratch.pforce[i];
      ps.vforce[i] = scratch.vforce[i];
      ps.eforce[i] = scratch.eforce[i];
      ps.density[i] = scratch.density[i];
      ps.pressure[i] = scratch.pressure[i];
    }
  }
}
