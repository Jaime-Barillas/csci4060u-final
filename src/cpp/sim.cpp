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
#include <utility>


Sim::Sim(std::filesystem::path exe_path, uint32_t particle_count, bool bench_mode)
: sim_opts{bench_mode, particle_count, PARTICLE_RADIUS, GAS_CONSTANT, REST_DENSITY, SUPPORT, VISCOSITY_CONSTANT},
  exe_path{exe_path},
  timer(BENCH_LENGTH) {
    ps_primary = new Particles();
    ps_secondary = new Particles();

    ps_primary->resize(sim_opts.particle_count);
    ps_secondary->resize(sim_opts.particle_count);
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
}

void Sim::update() {
  // 1. Model View matrix.
  // degrees += 0.025f;
  sdl_ctx->uniforms.gen_point_sprites.model_view = libcommon::matrix::translate_z(2.0f)
                                                 * libcommon::matrix::rotation_x(-20);

  // 2. Simulation.
  ns.process(*ps_primary, *ps_secondary, sim_opts);
  std::swap(ps_primary, ps_secondary);
  particles::calculate_density_pressure(*ps_primary, *ps_secondary, ns, sim_opts);
  std::swap(ps_primary, ps_secondary);
  particles::calculate_pressure_forces(*ps_primary, *ps_secondary, ns, sim_opts);
  particles::calculate_viscosity_forces(*ps_primary, *ps_secondary, ns, sim_opts);
  particles::calculate_external_forces(*ps_primary, *ps_secondary);
  particles::integrate(*ps_secondary);
  std::swap(ps_primary, ps_secondary);
}

void Sim::draw() {
  libcommon::draw(sdl_ctx, copy_particles, this);
}
