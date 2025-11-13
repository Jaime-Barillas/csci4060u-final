#include "particles.h"
#include <cmath>
#include <cstdint>
#include <ranges>
#include <vector>

void Particles::resize(size_t new_size) {
  pos.resize(new_size);
  vel.resize(new_size);
  pforce.resize(new_size);
  vforce.resize(new_size);
  eforce.resize(new_size);
  density.resize(new_size);
  pressure.resize(new_size);
}

void Particles::clear() {
  pos.clear();
  vel.clear();
  pforce.clear();
  vforce.clear();
  eforce.clear();
  density.clear();
  pressure.clear();
}

size_t Particles::size() const { return pos.size(); }

void Particles::reset(uint32_t count, float left_bound, float right_bound) {
  uint32_t length = (uint32_t)std::ceil(std::cbrt((float)count));
  float step = (right_bound - left_bound) * USABLE_SPACE_MODIFIER / length;
  float start = (-step * length) / 2.0f;

  resize(count);

  for (uint32_t i : std::views::iota(0u, count)) {
    uint32_t x = i % length;
    uint32_t y = (i / length) % length;
    uint32_t z = i / (length * length);
    pos[i] = Vec3{
      start + (x * step),
      start + (y * step),
      start + (z * step),
    };
    vel[i] = Vec3{0, 0, 0};
    pforce[i] = Vec3{0, 0, 0};
    vforce[i] = Vec3{0, 0, 0};
    eforce[i] = Vec3{0, 0, 0};
    density[i] = 0;
    pressure[i] = 0;
  }
}
