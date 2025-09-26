// #include "particles.h"
// #include <vector>
#include <filesystem>
#include <lib.h>
#include <print>
#include <SDL3/SDL_gpu.h>


bool update(libcommon::SDLCtx *ctx) {
  return libcommon::update(ctx);
}

void draw(libcommon::SDLCtx *ctx) {
  SDL_GPUCommandBuffer *render_cmds;
  SDL_GPUTexture *swapchain_tex;

  render_cmds = SDL_AcquireGPUCommandBuffer(ctx->device);
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(render_cmds, ctx->window, &swapchain_tex, nullptr, nullptr)) {
    SDL_CancelGPUCommandBuffer(render_cmds);
    return;
  }

  SDL_GPUColorTargetInfo cti = {
    .texture = swapchain_tex,
    .clear_color = { .r = 0.00, .g = 0.05, .b = 0.25 },
    .load_op = SDL_GPU_LOADOP_CLEAR,
    .store_op = SDL_GPU_STOREOP_STORE,
  };

  SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(render_cmds, &cti, 1, nullptr);
  SDL_EndGPURenderPass(render_pass);
  SDL_SubmitGPUCommandBuffer(render_cmds);
}

libcommon::SDLCtx *run_loop(libcommon::SDLCtx *ctx) {
  bool run = true;
  while (run) {
    run = ::update(ctx);
    draw(ctx);
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

  auto ctx = libcommon::initialize_and_setup(100)
    .transform(run_loop)
    .transform(libcommon::teardown);

  if (!ctx) {
    std::println("{}", ctx.error());
    return 1;
  }

  return 0;
}
