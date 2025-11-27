#pragma once

#include "neighbours.h"
#include "particles.h"
#include "sim_opts.h"
#include "timer.h"
#include <barrier>
#include <cstdint>
#include <filesystem>
#include <libcommon/lib.h>
#include <libcommon/vec.h>
#include <thread>
#include <vector>


enum Phase {
  DensityPressure,
  ForcesIntegrate,
  Exit,
};

struct WorkerData {
  size_t start;
  size_t end;
  Phase phase;
};

class Sim {
  std::filesystem::path exe_path;
  FrameTimer timer;

  SimOpts sim_opts;
  libcommon::SDLCtx *sdl_ctx;
  std::vector<std::thread> workers;
  std::vector<WorkerData> bounds;
  std::barrier<> begin;
  std::barrier<> complete;
  Particles *ps_primary;
  Particles *ps_secondary;

  Neighbours ns;

  static bool copy_particles(libcommon::SDLCtx *sdl_ctx, SDL_GPUTransferBuffer *tbuf, const void *sim_ctx);
  void worker_fn(size_t id);
  void update();
  void draw();

  public:
    Sim(std::filesystem::path exe_path, uint32_t particle_count, uint32_t thread_count, bool bench_mode);
    ~Sim();

    void init();
    void run_loop();
};
