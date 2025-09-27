// #include "particles.h"
#include <filesystem>
#include <lib.h>
#include <print>
#include <random>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <unistd.h>
#include <vector>


std::vector<float> particles;

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
  libcommon::draw(ctx, copy_particles, &particles);
}

libcommon::SDLCtx *run_loop(libcommon::SDLCtx *ctx) {
  libcommon::SDLError err{ .ctx = ctx, .type = libcommon::SDLErrorType::None };
  std::println("{}", err);


  ::draw(ctx);
  SDL_GPUTransferBufferCreateInfo tci = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
    // FIXME: undo
    .size = sizeof(float) * 4 * 6 * 128,
    .props = 0,
  };
  SDL_GPUBufferRegion source = {
    // particle pos' get correctly uploaded.
    // .buffer = ctx->bufs.point_sprites.b,
    .buffer = ctx->bufs.pass1.verts,
    .offset = 0,
    // FIXME: undo
    .size = sizeof(float) * 4 * 6 * 128,
  };
  SDL_GPUTransferBuffer *tb = SDL_CreateGPUTransferBuffer(ctx->device, &tci);
  SDL_GPUTransferBufferLocation dest = {.transfer_buffer = tb, .offset = 0};
  SDL_GPUCommandBuffer *c = SDL_AcquireGPUCommandBuffer(ctx->device);
  SDL_GPUCopyPass *p = SDL_BeginGPUCopyPass(c);
  SDL_DownloadFromGPUBuffer(p, &source, &dest);
  SDL_EndGPUCopyPass(p);
  SDL_GPUFence *f = SDL_SubmitGPUCommandBufferAndAcquireFence(c);
  bool res = SDL_WaitForGPUFences(ctx->device, true, &f, 1);
  if (!res) {std::println("failed to wait!");}
  float* m = (float*)SDL_MapGPUTransferBuffer(ctx->device, tb, true);
  std::println("og 1: ({}, {}, {})", particles[0], particles[1], particles[2]);
  std::println("og 2: ({}, {}, {})", particles[3], particles[4], particles[5]);
  std::println("og 3: ({}, {}, {})", particles[6], particles[7], particles[8]);
  std::println("============");
  std::println("1: ({}, {}, {}, {})", m[0], m[1], m[2], m[3]);
  std::println("2: ({}, {}, {}, {})", m[4], m[5], m[6], m[7]);
  std::println("3: ({}, {}, {}, {})", m[8], m[9], m[10], m[11]);
  std::println("5: ({}, {}, {}, {})", m[16], m[17], m[18], m[19]);
  SDL_UnmapGPUTransferBuffer(ctx->device, tb);
  SDL_ReleaseGPUFence(ctx->device, f);
  SDL_ReleaseGPUTransferBuffer(ctx->device, tb);


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
  std::random_device rdev;
  std::mt19937 rgen(rdev());
  std::uniform_real_distribution<float> rand(-1.0, 1.0);
  particles.reserve(128 * 4);
  for (int i = 0; i < 128 * 4; i++) {
    particles.push_back(rand(rgen));
  }

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
