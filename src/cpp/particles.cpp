#include "particles.h"
#include "util.h"
#include <algorithm>
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

  void calculate_density_pressure(Particles &ps) {
    size_t particle_count = ps.pos.size();

    for (size_t i = 0; i < particle_count; i++) {
      ps.density[i] = 0.0;
      ps.pressure[i] = 0.0;
      for (size_t j = 0; j < particle_count; j++) {
        ps.density[i] += kernel<PolyKernel>(ps.pos[i], ps.pos[j]);
      }
      ps.pressure[i] = GAS_CONSTANT * (ps.density[i] - REST_DENSITY);
    }
  }

  void calculate_pressure_forces(Particles &ps) {
    // FIXME: Something is wrong with the calculation.
    //        Particles tend to get 'sucked' into each other.
    size_t particle_count = ps.pos.size();

    Vec3 pressure_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      Vec3 pressure_temp = { 0, 0, 0 };

      for (size_t j = 0; j < particle_count; j++) {
        pressure_kernel_temp = kernel<SpikyGradKernel>(ps.pos[i], ps.pos[j]);
        float pressure_factor = (ps.pressure[i] + ps.pressure[j]) / (2 * ps.density[j]);
        pressure_temp.x += pressure_kernel_temp.x * pressure_factor;
        pressure_temp.y += pressure_kernel_temp.y * pressure_factor;
        pressure_temp.z += pressure_kernel_temp.z * pressure_factor;
      }

      ps.pforce[i] = pressure_temp;
    }
  }

  void calculate_viscosity_forces(Particles &ps) {
    size_t particle_count = ps.pos.size();
    
    float viscosity_kernel_temp;
    for (size_t i = 0; i < particle_count; i++) {
      particles::Vec3 viscosity_temp = { 0, 0, 0 };

      for (size_t j = 0; j < particle_count; j++) {
        viscosity_kernel_temp = kernel<ViscLaplKernel>(ps.pos[i], ps.pos[j]);
        float viscosity_factor_x = (ps.vel[j].x - ps.vel[i].x) / ps.density[j];
        float viscosity_factor_y = (ps.vel[j].y - ps.vel[i].y) / ps.density[j];
        float viscosity_factor_z = (ps.vel[j].z - ps.vel[i].z) / ps.density[j];
        viscosity_temp.x += VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_x;
        viscosity_temp.y += VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_y;
        viscosity_temp.z += VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_z;
      }

      ps.vforce[i] = viscosity_temp;
    }
  }

  void calculate_external_forces(Particles &ps) {
    size_t particle_count = ps.pos.size();

    for (size_t i = 0; i < particle_count; i++) {
      /*
      ps.eforce[i] = ps.pos[i].normalized();
      ps.eforce[i].negate();
      ps.eforce[i] *= particles::GRAVITY_STRENGTH;
      */
      bool flow_up = ps.pos[i].y < 0
                   && std::abs(ps.pos[i].x) < FOUNTAIN_WIDTH
                   && std::abs(ps.pos[i].z) < FOUNTAIN_WIDTH;
      ps.eforce[i] = { 0, -GRAVITY_STRENGTH, 0 };
      if (flow_up) {
        ps.eforce[i].y = GRAVITY_STRENGTH * FOUNTAIN_STRENGTH;
      }
    }
  }

  void integrate(Particles &ps) {
    size_t particle_count = ps.pos.size();

    particles::Vec3 acceleration;
    for (size_t i = 0; i < particle_count; i++) {
      // F = ma <=> a = F/m, m = 1.0 => a = F
      acceleration.x = ps.pforce[i].x + ps.vforce[i].x + ps.eforce[i].x;
      acceleration.y = ps.pforce[i].y + ps.vforce[i].y + ps.eforce[i].y;
      acceleration.z = ps.pforce[i].z + ps.vforce[i].z + ps.eforce[i].z;

      // v = a * dt;
      ps.vel[i].x += acceleration.x * (1.0f / 60); // FIXME: actually use delta time.
      ps.vel[i].y += acceleration.y * (1.0f / 60);
      ps.vel[i].z += acceleration.z * (1.0f / 60);

      // d = v * dt;
      ps.pos[i].x += ps.vel[i].x * (1.0f / 60);
      ps.pos[i].y += ps.vel[i].y * (1.0f / 60);
      ps.pos[i].z += ps.vel[i].z * (1.0f / 60);

      // Boundary conditions.
      if (ps.pos[i].x < LEFT_BOUND || ps.pos[i].x > RIGHT_BOUND) {
        ps.pos[i].x = std::clamp<float>(ps.pos[i].x, LEFT_BOUND, RIGHT_BOUND);
        ps.vel[i].x *= -0.5;
      }
      if (ps.pos[i].y < LOWER_BOUND || ps.pos[i].y > UPPER_BOUND) {
        ps.pos[i].y = std::clamp<float>(ps.pos[i].y, LOWER_BOUND, UPPER_BOUND);
        ps.vel[i].y *= -0.5;
      }
      if (ps.pos[i].z < BACKWARD_BOUND || ps.pos[i].z > FORWARD_BOUND) {
        ps.pos[i].z = std::clamp<float>(ps.pos[i].z, BACKWARD_BOUND, FORWARD_BOUND);
        ps.vel[i].z *= -0.5;
      }
    }
  }
}
