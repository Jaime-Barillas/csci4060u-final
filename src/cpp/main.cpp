#include "particles.h"
#include <algorithm>
#include <filesystem>
#include <lib.h>
#include <matrix.h>
#include <print>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <unistd.h>
#include <vector>


constexpr uint32_t PARTICLE_COUNT = 1024;
constexpr float LEFT_BOUND = -1.0;
constexpr float RIGHT_BOUND = 1.0;
constexpr float LOWER_BOUND = -1.0;
constexpr float UPPER_BOUND = 1.0;
constexpr float BACKWARD_BOUND = -1.0;
constexpr float FORWARD_BOUND = 1.0;
std::vector<particles::Particle> ps;
float degrees = 0;
uint64_t frame_counter = 0;
float gravity_dir = -1;

bool copy_particles(libcommon::SDLCtx *ctx, SDL_GPUTransferBuffer *tbuf, const void *particles_obj) {
  if (!particles_obj) {
    return false;
  }

  const std::vector<particles::Particle> *ps = static_cast<const std::vector<particles::Particle>*>(particles_obj);

  float *mapping = static_cast<float*>(SDL_MapGPUTransferBuffer(ctx->device, ctx->bufs.point_sprites.t, true));
  if (!mapping) {
    return false;
  }
  for (int i = 0; i < ps->size(); i++) {
    mapping[(i * 4) + 0] = ps->at(i).pos.x;
    mapping[(i * 4) + 1] = ps->at(i).pos.y;
    mapping[(i * 4) + 2] = ps->at(i).pos.z;
    mapping[(i * 4) + 3] = 1;
  }
  SDL_UnmapGPUTransferBuffer(ctx->device, ctx->bufs.point_sprites.t);

  return true;
}

bool update(libcommon::SDLCtx *ctx) {
  // 1. Model View matrix.
  // degrees += 0.02f;
  ctx->model_view = libcommon::matrix::translate_z(2.0f)
                  // * libcommon::matrix::rotation_x(-20)
                  * libcommon::matrix::rotation_y(degrees);

  // 2. Density + Pressure value.
  for (auto &p : ps) {
    p.density = 0.0;
    p.pressure = 0.0;
    for (auto &neighbour : ps) {
      p.density += particles::kernel<particles::PolyKernel>(p.pos, neighbour.pos);
    }
    p.pressure = particles::GAS_CONSTANT * (p.density - particles::REST_DENSITY);
  }

  // 3. Pressure forces.
  // FIXME: Something is wrong with the calculation.
  //        Particles tend to get 'sucked' into each other.
  particles::Vec3 pressure_kernel_temp;
  for (auto &p : ps) {
    particles::Vec3 pressure_temp = { 0, 0, 0 };
    for (auto &neighbour : ps) {
      pressure_kernel_temp = particles::kernel<particles::SpikyGradKernel>(p.pos, neighbour.pos);
      float pressure_factor = (p.pressure + neighbour.pressure) / (2 * neighbour.density);
      pressure_temp.x += pressure_kernel_temp.x * pressure_factor;
      pressure_temp.y += pressure_kernel_temp.y * pressure_factor;
      pressure_temp.z += pressure_kernel_temp.z * pressure_factor;
    }
    p.pforce = pressure_temp;
  }

  // 4. Viscosity forces.
  float viscosity_kernel_temp;
  for (auto &p : ps) {
    particles::Vec3 viscosity_temp = { 0, 0, 0 };
    for (auto &neighbour : ps) {
      viscosity_kernel_temp = particles::kernel<particles::ViscLaplKernel>(p.pos, neighbour.pos);
      float viscosity_factor_x = (neighbour.vel.x - p.vel.x) / neighbour.density;
      float viscosity_factor_y = (neighbour.vel.y - p.vel.y) / neighbour.density;
      float viscosity_factor_z = (neighbour.vel.z - p.vel.z) / neighbour.density;
      viscosity_temp.x += particles::VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_x;
      viscosity_temp.y += particles::VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_y;
      viscosity_temp.z += particles::VISCOSITY_CONSTANT * viscosity_kernel_temp * viscosity_factor_z;
    }
    p.vforce = viscosity_temp;
  }

  // 5. External forces.
  for (auto &p : ps) {
    /*
    p.eforce = p.pos.normalized();
    p.eforce.negate();
    p.eforce *= particles::GRAVITY_STRENGTH;
    */
    p.eforce = { 0, particles::GRAVITY_STRENGTH * gravity_dir, 0 };
  }

  // 6. Integrate.
  particles::Vec3 acceleration;
  for (auto &p : ps) {
    // F = ma <=> a = F/m, m = 1.0 => a = F
    acceleration.x = p.pforce.x + p.vforce.x + p.eforce.x;
    acceleration.y = p.pforce.y + p.vforce.y + p.eforce.y;
    acceleration.z = p.pforce.z + p.vforce.z + p.eforce.z;

    // v = a * dt;
    p.vel.x = acceleration.x * (1.0f / 60); // FIXME: actually use delta time.
    p.vel.y = acceleration.y * (1.0f / 60);
    p.vel.z = acceleration.z * (1.0f / 60);

    // d = v * dt;
    p.pos.x += p.vel.x * (1.0f / 60);
    p.pos.y += p.vel.y * (1.0f / 60);
    p.pos.z += p.vel.z * (1.0f / 60);

    // Boundary conditions.
    if (p.pos.x < LEFT_BOUND || p.pos.x > RIGHT_BOUND) {
      p.pos.x = std::clamp<float>(p.pos.x, LEFT_BOUND, RIGHT_BOUND);
      p.vel.x *= -0.5;
    }
    if (p.pos.y < LOWER_BOUND || p.pos.y > UPPER_BOUND) {
      p.pos.y = std::clamp<float>(p.pos.y, LOWER_BOUND, UPPER_BOUND);
      p.vel.y *= -0.5;
    }
    if (p.pos.z < BACKWARD_BOUND || p.pos.z > FORWARD_BOUND) {
      p.pos.z = std::clamp<float>(p.pos.z, BACKWARD_BOUND, FORWARD_BOUND);
      p.vel.z *= -0.5;
    }
  }

  frame_counter += 1;
  if (frame_counter % 300 == 0) {
    gravity_dir = -gravity_dir;
  }

  std::print("\x1b[1J\x1b[1;1H"); // Clear from cursor to start of screen + move cursor to top-left.
  std::print("\x1b[0GDensity(544): {}\n", ps[544].density);
  std::print("\x1b[0GPressure(544): {}\n", ps[544].pressure);
  std::print("\x1b[0GPressure Force(544): ({}, {}, {})\n", ps[544].pforce.x, ps[544].pforce.y, ps[544].pforce.z);
  std::print("\x1b[0GViscosity Force(544): ({}, {}, {})\n", ps[544].vforce.x, ps[544].vforce.y, ps[544].vforce.z);
  std::print("\x1b[0GExternal Force(544): ({}, {}, {})\n", ps[544].eforce.x, ps[544].eforce.y, ps[544].eforce.z);
  std::print("\x1b[0GVelocity(544): ({}, {}, {})\n", ps[544].vel.x, ps[544].vel.y, ps[544].vel.z);

  return libcommon::update(ctx);
}

void draw(libcommon::SDLCtx *ctx) {
  libcommon::draw(ctx, copy_particles, &ps);
}

libcommon::SDLCtx *run_loop(libcommon::SDLCtx *ctx) {
  libcommon::SDLError err{ .ctx = ctx, .type = libcommon::SDLErrorType::None };
  std::println("{}", err);

  particles::reset(ps, PARTICLE_COUNT, LEFT_BOUND, RIGHT_BOUND);

  int count = 0;
  bool run = true;
  std::print("\x1b[2J"); // Clear screen (println until clear)
  while (run) {
    run = ::update(ctx);
    ::draw(ctx);

    // Run longer than one frame to see non-zero values.
    // count++;
    // run = (count == 1) ? false : run;
  }

  return ctx;
}

int main(int argc, const char **argv) {
  std::filesystem::path exe_path(argv[0]);

  auto ctx = libcommon::initialize_and_setup(exe_path.parent_path().c_str(), PARTICLE_COUNT)
    .transform(run_loop)
    .transform(libcommon::teardown);

  if (!ctx) {
    std::println("{}", ctx.error());
    return 1;
  }

  return 0;
}
