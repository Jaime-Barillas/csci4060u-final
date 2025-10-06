#include "particles.h"
#include <filesystem>
#include <lib.h>
#include <matrix.h>
#include <print>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <unistd.h>
#include <vector>


constexpr uint32_t PARTICLE_COUNT = 256;
std::vector<particles::Particle> ps;
float degrees = 0;

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
  degrees += 0.3f;
  ctx->model_view = libcommon::matrix::translate_z(2.0f)
                  // * libcommon::matrix::rotation_x(-20)
                  * libcommon::matrix::rotation_y(degrees);
  return libcommon::update(ctx);
}

void draw(libcommon::SDLCtx *ctx) {
  libcommon::draw(ctx, copy_particles, &ps);
}

libcommon::SDLCtx *run_loop(libcommon::SDLCtx *ctx) {
  libcommon::SDLError err{ .ctx = ctx, .type = libcommon::SDLErrorType::None };
  std::println("{}", err);

  particles::reset(ps, PARTICLE_COUNT, -1.0, 1.0);

  bool run = true;
  while (run) {
    run = ::update(ctx);
    ::draw(ctx);
  }

  return ctx;

  // std::println("({}, {}, {})", ps[0].pos.x, ps[0].pos.y, ps[0].pos.z);
  // std::println("({}, {}, {})", ps[1].pos.x, ps[1].pos.y, ps[1].pos.z);
  // std::println("  Poly: {}", particles::kernel<particles::PolyKernel>(ps[0].pos, ps[1].pos));
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
