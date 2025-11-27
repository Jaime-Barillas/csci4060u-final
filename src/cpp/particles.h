#pragma once

#include <cstddef>
#include <cstdint>
#include <libcommon/vec.h>
#include <new>
#include <vector>

constexpr float USABLE_SPACE_MODIFIER = 0.8f;
constexpr float PARTICLE_RADIUS = 0.075f;
constexpr float SUPPORT = 0.33f;
constexpr float GAS_CONSTANT = 0.3f;
constexpr float REST_DENSITY = 300.0f;
constexpr float VISCOSITY_CONSTANT = 0.1f;
constexpr float GRAVITY_STRENGTH = 10.0f;
constexpr float FOUNTAIN_WIDTH = 0.25;
constexpr float FOUNTAIN_STRENGTH = 1.5;

// Simulation area bounds.
constexpr float LEFT_BOUND = -1.0;
constexpr float RIGHT_BOUND = 1.0;
constexpr float LOWER_BOUND = -1.0;
constexpr float UPPER_BOUND = 1.0;
constexpr float BACKWARD_BOUND = -1.0;
constexpr float FORWARD_BOUND = 1.0;

template <typename T>
struct AlignedAllocator {
  using value_type = T;

  T *allocate(size_t count) {
    if (count <= 0) { throw std::bad_alloc{}; }

    size_t size = sizeof(T) * count;
    return static_cast<T*>(::operator new(size, std::align_val_t(64)));
  }

  void deallocate(T *ptr, size_t count) noexcept {
    if (!ptr) return;

    size_t size = sizeof(T) * count;
    ::operator delete(ptr, size, std::align_val_t(64));
  }
};

struct Particles {
  std::vector<float /*, AlignedAllocator<float>*/> pos_x, pos_y, pos_z;
  std::vector<float /*, AlignedAllocator<float>*/> vel_x, vel_y, vel_z;
  std::vector<float /*, AlignedAllocator<float>*/> forces_x, forces_y, forces_z;
  std::vector<float /*, AlignedAllocator<float>*/> density;
  std::vector<float /*, AlignedAllocator<float>*/> pressure;

  void resize(size_t new_size);
  void clear();
  size_t size() const;
  void reset(uint32_t count, float left_bound, float right_bound);
};
