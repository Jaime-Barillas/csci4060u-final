// #include "particles.h"
// #include <vector>
#include "SDL3/SDL_stdinc.h"
#include <filesystem>
#include <lib.h>
#include <print>
#include <SDL3/SDL_gpu.h>
#include <vector>


bool copy_particles(libcommon::SDLCtx *ctx, SDL_GPUTransferBuffer *tbuf, const void *particles_obj) {
  if (!particles_obj) {
    return false;
  }

  const std::vector<float> *ps = static_cast<const std::vector<float>*>(particles_obj);

  void *mapping = (SDL_MapGPUTransferBuffer(ctx->device, ctx->bufs.point_sprites.t, true));
  if (!mapping) {
    return false;
  }
  SDL_memcpy(mapping, ps->data(), sizeof(float) * ps->size());
  SDL_UnmapGPUTransferBuffer(ctx->device, ctx->bufs.point_sprites.t);

  return true;
}

bool update(libcommon::SDLCtx *ctx) {
  return libcommon::update(ctx);
}

void draw(libcommon::SDLCtx *ctx) {
  libcommon::draw(ctx, copy_particles, nullptr);
}

libcommon::SDLCtx *run_loop(libcommon::SDLCtx *ctx) {
  bool run = true;
  while (run) {
    run = ::update(ctx);
    ::draw(ctx);
  }

  return ctx;

  // std::vector<Particle> ps;

  // particles::reset(ps, particles::DEFAULT_PARTICLE_COUNT, -100, 100);
  // std::println("({}, {}, {})", ps[0].pos.x, ps[0].pos.y, ps[0].pos.z);
  // std::println("({}, {}, {})", ps[1].pos.x, ps[1].pos.y, ps[1].pos.z);
  // std::println("  Poly: {}", particles::kernel<particles::PolyKernel>(ps[0].pos, ps[1].pos));
}

int main(int argc, const char **argv) {
  std::filesystem::path exe_path(argv[0]);

  auto ctx = libcommon::initialize_and_setup(exe_path.parent_path().c_str(), 128)
    .transform(run_loop)
    .transform(libcommon::teardown);

  if (!ctx) {
    std::println("{}", ctx.error());
    return 1;
  }

  return 0;
}
