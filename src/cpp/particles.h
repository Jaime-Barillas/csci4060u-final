#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <libcommon/vec.h>
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

struct Particles {
  std::vector<Vec3> pos;
  std::vector<Vec3> vel;
  std::vector<Vec3> pforce; // Pressure forces
  std::vector<Vec3> vforce; // Viscosity forces
  std::vector<Vec3> eforce; // External forces
  std::vector<float> density;
  std::vector<float> pressure;

  void resize(size_t new_size);
  void clear();
  size_t size() const;
  void reset(uint32_t count, float left_bound, float right_bound);
};
