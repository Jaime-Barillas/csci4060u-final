#pragma once

#include "particles.h"
#include "timer.h"
#include <cstdint>
#include <filesystem>
#include <libcommon/lib.h>
#include <libcommon/vec.h>


constexpr uint32_t BENCH_LENGTH = 300; // frames
constexpr Vec2 X_BOUNDS{-1.0f, 1.0f};
constexpr Vec2 Y_BOUNDS{-1.0f, 1.0f};
constexpr Vec2 Z_BOUNDS{-1.0f, 1.0f};

struct SimOpts {
  bool bench_mode;
  uint32_t particle_count;
  float particle_radius;
};

class Sim {
  std::filesystem::path exe_path;
  FrameTimer timer;

  SimOpts sim_opts;
  libcommon::SDLCtx *sdl_ctx;
  particles::Particles ps;

  static bool copy_particles(libcommon::SDLCtx *sdl_ctx, SDL_GPUTransferBuffer *tbuf, const void *sim_ctx);
  void update();
  void draw();

  public:
    Sim(std::filesystem::path exe_path, uint32_t particle_count, bool bench_mode);

    void init();
    void run_loop();
};
