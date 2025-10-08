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
constexpr float LEFT_BOUND = -1.0;
constexpr float RIGHT_BOUND = 1.0;
constexpr float LOWER_BOUND = -1.0;
constexpr float UPPER_BOUND = 1.0;
constexpr float BACKWARD_BOUND = -1.0;
constexpr float FORWARD_BOUND = 1.0;
particles::Particles ps;
float degrees = 0;
uint64_t frame_counter = 1;
float gravity_dir = -1;

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
  // degrees += 0.02f;
  ctx->uniforms.gen_point_sprites.model_view = libcommon::matrix::translate_z(2.0f)
                                             // * libcommon::matrix::rotation_x(-20)
                                             * libcommon::matrix::rotation_y(degrees);

  // 2. Density + Pressure value.
  for (size_t i = 0; i < ps.pos.size(); i++) {
    ps.density[i] = 0.0;
    ps.pressure[i] = 0.0;
    for (size_t j = 0; j < ps.pos.size(); j++) {
      ps.density[i] += particles::kernel<particles::PolyKernel>(ps.pos[i], ps.pos[j]);
    }
    ps.pressure[i] = particles::GAS_CONSTANT * (ps.density[i] - particles::REST_DENSITY);
  }

  // 3. Pressure forces.
  // FIXME: Something is wrong with the calculation.
  //        Particles tend to get 'sucked' into each other.
  particles::Vec3 pressure_kernel_temp;
  for (size_t i = 0; i < ps.pos.size(); i++) {
    particles::Vec3 pressure_temp = { 0, 0, 0 };
    for (size_t j = 0; j < ps.pos.size(); j++) {
      pressure_kernel_temp = particles::kernel<particles::SpikyGradKernel>(ps.pos[i], ps.pos[j]);
      float pressure_factor = (ps.pressure[i] + ps.pressure[j]) / (2 * ps.density[j]);
      pressure_temp.x += pressure_kernel_temp.x * pressure_factor;
      pressure_temp.y += pressure_kernel_temp.y * pressure_factor;
      pressure_temp.z += pressure_kernel_temp.z * pressure_factor;
    }
    ps.pforce[i] = pressure_temp;
  }

  // 4. Viscosity forces.
  float viscosity_kernel_temp;
  for (size_t i = 0; i < ps.pos.size(); i++) {
    particles::Vec3 viscosity_temp = { 0, 0, 0 };
    for (size_t j = 0; j < ps.pos.size(); j++) {
      viscosity_kernel_temp = particles::kernel<particles::ViscLaplKernel>(ps.pos[i], ps.pos[j]);
      float viscosity_factor_x = (ps.vel[j].x - ps.vel[i].x) / ps.density[j];
      float viscosity_factor_y = (ps.vel[j].y - ps.vel[i].y) / ps.density[j];
      float viscosity_factor_z = (ps.vel[j].z - ps.vel[i].z) / ps.density[j];
      viscosity_temp.x += particles::VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_x;
      viscosity_temp.y += particles::VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_y;
      viscosity_temp.z += particles::VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_z;
    }
    ps.vforce[i] = viscosity_temp;
  }

  // 5. External forces.
  for (size_t i = 0; i < ps.pos.size(); i++) {
    /*
    ps.eforce[i] = ps.pos[i].normalized();
    ps.eforce[i].negate();
    ps.eforce[i] *= particles::GRAVITY_STRENGTH;
    */
    ps.eforce[i] = { 0, particles::GRAVITY_STRENGTH * gravity_dir, 0 };
  }

  // 6. Integrate.
  particles::Vec3 acceleration;
  for (size_t i = 0; i < ps.pos.size(); i++) {
    // F = ma <=> a = F/m, m = 1.0 => a = F
    acceleration.x = ps.pforce[i].x + ps.vforce[i].x + ps.eforce[i].x;
    acceleration.y = ps.pforce[i].y + ps.vforce[i].y + ps.eforce[i].y;
    acceleration.z = ps.pforce[i].z + ps.vforce[i].z + ps.eforce[i].z;

    // v = a * dt;
    ps.vel[i].x = acceleration.x * (1.0f / 60); // FIXME: actually use delta time.
    ps.vel[i].y = acceleration.y * (1.0f / 60);
    ps.vel[i].z = acceleration.z * (1.0f / 60);

    // d = v * dt;
    ps.pos[i].x += ps.vel[i].x * (1.0f / 60);
    ps.pos[i].y += ps.vel[i].y * (1.0f / 60);
    ps.pos[i].z += ps.vel[i].z * (1.0f / 60);

    // Boundary conditions.
    if (ps.pos[i].x < LEFT_BOUND || ps.pos[i].x > RIGHT_BOUND) {
      ps.pos[i].x = std::clamp<float>(ps.pos[i].x, LEFT_BOUND, RIGHT_BOUND);
      ps.vel[i].x *= -0.5;
    }
    if (ps.pos[i].y < LOWER_BOUND || ps.pos[i].y > UPPER_BOUND) {
      ps.pos[i].y = std::clamp<float>(ps.pos[i].y, LOWER_BOUND, UPPER_BOUND);
      ps.vel[i].y *= -0.5;
    }
    if (ps.pos[i].z < BACKWARD_BOUND || ps.pos[i].z > FORWARD_BOUND) {
      ps.pos[i].z = std::clamp<float>(ps.pos[i].z, BACKWARD_BOUND, FORWARD_BOUND);
      ps.vel[i].z *= -0.5;
    }
  }

  if (frame_counter % 300 == 0) {
    gravity_dir = -gravity_dir;
  }

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
  particles::reset(ps, PARTICLE_COUNT, LEFT_BOUND, RIGHT_BOUND);

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
