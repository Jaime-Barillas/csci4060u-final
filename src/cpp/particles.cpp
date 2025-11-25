#include "particles.h"
#include <cmath>
#include <cstdint>
#include <ranges>
#include <vector>

void Particles::resize(size_t new_size) {
  pos_x.resize(new_size);
  pos_y.resize(new_size);
  pos_z.resize(new_size);
  vel_x.resize(new_size);
  vel_y.resize(new_size);
  vel_z.resize(new_size);
  forces_x.resize(new_size);
  forces_y.resize(new_size);
  forces_z.resize(new_size);
  density.resize(new_size);
  pressure.resize(new_size);
}

void Particles::clear() {
  pos_x.clear();
  pos_y.clear();
  pos_z.clear();
  vel_x.clear();
  vel_y.clear();
  vel_z.clear();
  forces_x.clear();
  forces_y.clear();
  forces_z.clear();
  density.clear();
  pressure.clear();
}

size_t Particles::size() const { return pos_x.size(); }

void Particles::reset(uint32_t count, float left_bound, float right_bound) {
  uint32_t length = (uint32_t)std::ceil(std::cbrt((float)count));
  float step = (right_bound - left_bound) * USABLE_SPACE_MODIFIER / length;
  float start = (-step * length) / 2.0f;

  resize(count);

  for (uint32_t i : std::views::iota(0u, count)) {
    uint32_t x = i % length;
    uint32_t y = (i / length) % length;
    uint32_t z = i / (length * length);

    pos_x[i] = start + (x * step);
    pos_y[i] = start + (y * step);
    pos_z[i] = start + (z * step);
    vel_x[i] = 0;
    vel_y[i] = 0;
    vel_z[i] = 0;
    forces_x[i] = 0;
    forces_y[i] = 0;
    forces_z[i] = 0;
    density[i] = 0;
    pressure[i] = 0;
  }
}
