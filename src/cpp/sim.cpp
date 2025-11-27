#include "sim.h"

#include "neighbours.h"
#include "particles.h"
#include "procs.h"
#include "timer.h"
#include <cstdint>
#include <filesystem>
#include <format>
#include <libcommon/lib.h>
#include <libcommon/matrix.h>
#include <libcommon/vec.h>
#include <print>
#include <SDL3/SDL_gpu.h>
#include <stdexcept>
#include <thread>
#include <utility>


Sim::Sim(std::filesystem::path exe_path, uint32_t particle_count, uint32_t thread_count, bool bench_mode)
: sim_opts{bench_mode, particle_count, PARTICLE_RADIUS, GAS_CONSTANT, REST_DENSITY, SUPPORT, VISCOSITY_CONSTANT},
  exe_path{exe_path},
  begin(thread_count + 1),
  complete(thread_count + 1),
  timer(BENCH_LENGTH) {
    ps_primary = new Particles();
    ps_secondary = new Particles();

    ps_primary->resize(sim_opts.particle_count);
    ps_secondary->resize(sim_opts.particle_count);

    size_t chunk_size = sim_opts.particle_count / thread_count;
    for (size_t i = 0; i < thread_count; i++) {
      size_t chunk_start = chunk_size * i;
      size_t chunk_end = chunk_size * (i + 1);
      if (i == thread_count - 1) {
        chunk_end = sim_opts.particle_count;
      }
      bounds.push_back({chunk_start, chunk_end, ForcesIntegrate});
      workers.push_back(std::thread(&Sim::worker_fn, this, i));
    }
}

Sim::~Sim() {
  delete ps_primary;
  delete ps_secondary;
}

bool Sim::copy_particles(libcommon::SDLCtx *sdl_ctx, SDL_GPUTransferBuffer *tbuf, const void *sim_ctx) {
  if (!sim_ctx) {
    return false;
  }

  Vec4 *mapping = static_cast<Vec4*>(SDL_MapGPUTransferBuffer(sdl_ctx->device, sdl_ctx->bufs.point_sprites.t, true));
  if (!mapping) {
    return false;
  }

  const Sim *sim = static_cast<const Sim*>(sim_ctx);
  for (int i = 0; i < sim->sim_opts.particle_count; i++) {
    mapping[i].data[0] = sim->ps_primary->pos_x[i];
    mapping[i].data[1] = sim->ps_primary->pos_y[i];
    mapping[i].data[2] = sim->ps_primary->pos_z[i];
  }

  SDL_UnmapGPUTransferBuffer(sdl_ctx->device, sdl_ctx->bufs.point_sprites.t);

  return true;
}

void Sim::init() {
  auto res = libcommon::initialize_and_setup(exe_path.parent_path().c_str(), sim_opts.particle_count);

  if (!res) {
    throw std::runtime_error(std::format("{}", res.error()));
  }

  sdl_ctx = res.value();
  sdl_ctx->uniforms.gen_point_sprites.particle_radius = sim_opts.particle_radius;
  ps_primary->reset(sim_opts.particle_count, X_BOUNDS.x(), X_BOUNDS.y());
}

void Sim::run_loop() {
  uintptr_t ptr_primary = reinterpret_cast<uintptr_t>(ps_primary->pos_x.data());
  size_t offset = ptr_primary % 64;
  std::println("Offset: {}", offset);

  bool run = true;
  while (run) {
    run = libcommon::update(sdl_ctx);

    timer.record_start();
    update();
    timer.record_end();
    draw();

    if (sim_opts.bench_mode && timer.recorded_frames() == BENCH_LENGTH) {
      run = false;
    }
  }

  if (sim_opts.bench_mode) {
    std::println("{}", timer.average_millis());
  }

  for (auto &worker_data : bounds) {
    worker_data.phase = Exit;
  }
  begin.arrive();
  complete.arrive();
  for (auto &w : workers) {
    w.join();
  }
}

void Sim::worker_fn(size_t id) {
  std::println("{}: start: {}, end: {}", id, bounds[id].start, bounds[id].end);
  while(true) {
    begin.arrive_and_wait();

    switch (bounds[id].phase) {
      case DensityPressure:
        particles::calculate_density_pressure(*ps_primary, *ps_secondary, ns, bounds[id].start, bounds[id].end, sim_opts);
        break;
      case ForcesIntegrate:
        particles::calculate_pressure_forces(*ps_primary, *ps_secondary, ns, bounds[id].start, bounds[id].end, sim_opts);
        particles::calculate_viscosity_forces(*ps_primary, *ps_secondary, ns, bounds[id].start, bounds[id].end, sim_opts);
        particles::calculate_external_forces(*ps_primary, *ps_secondary, bounds[id].start, bounds[id].end);
        particles::integrate(*ps_secondary, bounds[id].start, bounds[id].end);
        break;
      case Exit:
        goto exit;
        break;
    }
    complete.arrive();
  }

  exit:
  complete.arrive();
}

void Sim::update() {
  // Model View matrix.
  sdl_ctx->uniforms.gen_point_sprites.model_view = libcommon::matrix::translate_z(2.0f)
                                                 * libcommon::matrix::rotation_x(-20);

  // Particle data is stored in two buffers. Each computation reads from the
  // primary buffer and writes to the secondary buffer. After each "phase" of
  // computation, the pointers to the buffers are swapped. Additionally, the
  // buffers are in SoA layout which should allow chunking the data easily
  // for the multiple threads.

  // There are three phases of computation for the simulation. After each
  // phase, all threads of computation are synchronised to make computed
  // values available to later phases. The first phase is currently not
  // multithreaded.

  // First: The particles are sorted according to which 3D grid cell they
  //        belong to.
  ns.process(*ps_primary, *ps_secondary, sim_opts);
  std::swap(ps_primary, ps_secondary);

  // Second: Density and pressure values are calculated for each particle.
  for (auto &worker_data : bounds) {
    worker_data.phase = DensityPressure;
  }
  begin.arrive();
  complete.arrive_and_wait();
  std::swap(ps_primary, ps_secondary);

  // Third: Forces are calculated and particle positions are advanced. These
  //        computations depend on the values calculated in the second phase.
  for (auto &worker_data : bounds) {
    worker_data.phase = ForcesIntegrate;
  }
  begin.arrive();
  complete.arrive_and_wait();
  std::swap(ps_primary, ps_secondary);
}

void Sim::draw() {
  libcommon::draw(sdl_ctx, copy_particles, this);
}

