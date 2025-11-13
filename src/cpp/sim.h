#pragma once

#include "neighbours.h"
#include "particles.h"
#include "sim_opts.h"
#include "timer.h"
#include <cstdint>
#include <filesystem>
#include <libcommon/lib.h>
#include <libcommon/vec.h>


class Sim {
  std::filesystem::path exe_path;
  FrameTimer timer;

  SimOpts sim_opts;
  libcommon::SDLCtx *sdl_ctx;
  Particles ps;

  Neighbours ns;

  static bool copy_particles(libcommon::SDLCtx *sdl_ctx, SDL_GPUTransferBuffer *tbuf, const void *sim_ctx);
  void update();
  void draw();

  public:
    Sim(std::filesystem::path exe_path, uint32_t particle_count, bool bench_mode);

    void init();
    void run_loop();
};
