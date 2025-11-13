#pragma once

#include <cstdint>
#include <libcommon/vec.h>

constexpr uint32_t BENCH_LENGTH = 300; // frames
constexpr Vec2 X_BOUNDS{-1.0f, 1.0f};
constexpr Vec2 Y_BOUNDS{-1.0f, 1.0f};
constexpr Vec2 Z_BOUNDS{-1.0f, 1.0f};

struct SimOpts {
  bool bench_mode;
  uint32_t particle_count;
  float particle_radius;
  float gas_constant;
  float rest_density;
  float support;
  float viscosity_constant;
};
