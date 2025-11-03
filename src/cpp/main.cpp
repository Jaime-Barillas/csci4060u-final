#include "particles.h"
#include <cstdint>
#include <filesystem>
#include <lib.h>
#include <matrix.h>
#include <numeric>
#include <print>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <unistd.h>
#include <vector>


constexpr uint32_t BENCH_LENGTH = 300; // frames
constexpr uint32_t PARTICLE_COUNT = 1024;
particles::Particles ps;
float degrees = 0;
uint64_t frame_counter = 1;

bool bench_mode = false;
uint64_t frame_index = 0;
std::vector<float> frame_times(BENCH_LENGTH);

bool copy_particles(libcommon::SDLCtx *ctx, SDL_GPUTransferBuffer *tbuf, const void *particles_obj) {
  if (!particles_obj) {
    return false;
  }

  const particles::Particles *ps = static_cast<const particles::Particles*>(particles_obj);

  float *mapping = static_cast<float*>(SDL_MapGPUTransferBuffer(ctx->device, ctx->bufs.point_sprites.t, true));
  if (!mapping) {
    return false;
  }
  for (int i = 0; i < ps->pos.size(); i++) {
    mapping[(i * 4) + 0] = ps->pos.at(i).x;
    mapping[(i * 4) + 1] = ps->pos.at(i).y;
    mapping[(i * 4) + 2] = ps->pos.at(i).z;
    mapping[(i * 4) + 3] = 1;
  }
  SDL_UnmapGPUTransferBuffer(ctx->device, ctx->bufs.point_sprites.t);

  return true;
}

bool update(libcommon::SDLCtx *ctx) {
  // 1. Model View matrix.
  degrees += 0.025f;
  ctx->uniforms.gen_point_sprites.model_view = libcommon::matrix::translate_z(2.0f)
                                             * libcommon::matrix::rotation_x(-20)
                                             * libcommon::matrix::rotation_y(degrees);

  // Simulation.
  particles::count_sort(ps);
  particles::calculate_density_pressure(ps);
  particles::calculate_pressure_forces(ps);
  particles::calculate_viscosity_forces(ps);
  particles::calculate_external_forces(ps);
  particles::integrate(ps);

  if (!bench_mode) {
    std::print("\x1b[1J\x1b[1;1H"); // Clear from cursor to start of screen + move cursor to top-left.
    std::print("\x1b[0GDensity(544): {}\n", ps.density[544]);
    std::print("\x1b[0GPressure(544): {}\n", ps.pressure[544]);
    std::print("\x1b[0GPressure Force(544): ({}, {}, {})\n", ps.pforce[544].x, ps.pforce[544].y, ps.pforce[544].z);
    std::print("\x1b[0GViscosity Force(544): ({}, {}, {})\n", ps.vforce[544].x, ps.vforce[544].y, ps.vforce[544].z);
    std::print("\x1b[0GExternal Force(544): ({}, {}, {})\n", ps.eforce[544].x, ps.eforce[544].y, ps.eforce[544].z);
    std::print("\x1b[0GVelocity(544): ({}, {}, {})\n", ps.vel[544].x, ps.vel[544].y, ps.vel[544].z);

    float size = (frame_counter < BENCH_LENGTH) ? frame_counter : frame_times.size();
    float avg = std::reduce(frame_times.begin(), frame_times.end()) / size;
    std::print("\n\x1b[0GAverage Frame Time: {}s\n", avg);
  }

  return libcommon::update(ctx);
}

void draw(libcommon::SDLCtx *ctx) {
  libcommon::draw(ctx, copy_particles, &ps);
}

libcommon::SDLCtx *run_loop(libcommon::SDLCtx *ctx) {
  if (!bench_mode) {
    libcommon::SDLError err{ .ctx = ctx, .type = libcommon::SDLErrorType::None };
    std::println("{}", err);
    std::print("\x1b[2J"); // Clear screen (println until clear)
  }

  ctx->uniforms.gen_point_sprites.particle_radius = particles::PARTICLE_RADIUS;
  particles::reset(ps, PARTICLE_COUNT, particles::LEFT_BOUND, particles::RIGHT_BOUND);

  bool run = true;
  uint64_t update_start = 0;
  uint64_t update_time = 0;
  while (run) {

    update_start = SDL_GetPerformanceCounter();
    run = ::update(ctx);
    update_time = SDL_GetPerformanceCounter() - update_start;
    frame_times[frame_index % BENCH_LENGTH] = static_cast<float>(update_time) / SDL_GetPerformanceFrequency();

    ::draw(ctx);

    if (bench_mode && frame_counter == BENCH_LENGTH) {
      run = false;
    }

    frame_counter += 1;
    frame_index += 1;
  }

  return ctx;
}

int main(int argc, const char **argv) {
  std::filesystem::path exe_path(argv[0]);

  if (argc >= 2 && std::string_view(argv[1]) == "--bench") {
    bench_mode = true;
  }

  auto ctx = libcommon::initialize_and_setup(exe_path.parent_path().c_str(), PARTICLE_COUNT)
    .transform(run_loop)
    .transform(libcommon::teardown);

  if (!ctx) {
    std::println("{}", ctx.error());
    return 1;
  } else if (bench_mode) {
    float avg = std::reduce(frame_times.begin(), frame_times.end()) / frame_times.size();
    std::println("{}", avg);
  }

  return 0;
}
