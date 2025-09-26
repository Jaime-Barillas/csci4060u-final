#include "lib.h"
#include "Vec3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

namespace libcommon {
  using libcommon_return_t = std::expected<SDLCtx*, SDLError>;

  libcommon_return_t _init_sdl(uint32_t particle_count) {
    if (particle_count == 0) {
      return std::unexpected((SDLError){
        .ctx = nullptr,
        .type = SDLErrorType::BadParticleCount,
      });
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
      return std::unexpected((SDLError){
        .ctx = nullptr,
        .type = SDLErrorType::Initialization,
      });
    }

    return new SDLCtx{ .particle_count = particle_count };
  }

  SDLError _quit_sdl(SDLError error) {
    if (error.type > SDLErrorType::Initialization) {
      SDL_Quit();
      delete error.ctx;
      error.ctx = nullptr;
    }

    return error;
  }

  libcommon_return_t _create_window(SDLCtx *ctx) {
    SDL_DisplayID display;
    SDL_Rect bounds;

    display = SDL_GetPrimaryDisplay();
    if (display == 0) {
      bounds.w = 720;
      bounds.h = 720;
    } else {
      SDL_GetDisplayBounds(display, &bounds);
    }

    bounds.w = (bounds.w * 2) / 3;
    bounds.h = (bounds.h * 2) / 3;

    ctx->window = SDL_CreateWindow("fluid-sim-sph", bounds.w, bounds.h, 0);
    if (!ctx->window) {
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::WindowCreation,
      });
    }

    return ctx;
  }

  SDLError _destroy_window(SDLError error) {
    if (error.type > SDLErrorType::WindowCreation) {
      SDL_DestroyWindow(error.ctx->window);
    }

    return error;
  }

  libcommon_return_t _create_gpu_device_and_claim_window(SDLCtx *ctx) {
    ctx->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr);
    if (!ctx->device) {
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::GpuDeviceCreation,
      });
    }

    if (!SDL_ClaimWindowForGPUDevice(ctx->device, ctx->window)) {
      SDL_DestroyGPUDevice(ctx->device);
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::ClaimingWindow,
      });
    }

    return ctx;
  }

  SDLError _release_window_and_destroy_gpu_device(SDLError error) {
    if (error.type > SDLErrorType::ClaimingWindow) {
      SDL_ReleaseWindowFromGPUDevice(error.ctx->device, error.ctx->window);
    }

    if (error.type > SDLErrorType::GpuDeviceCreation) {
      SDL_DestroyGPUDevice(error.ctx->device);
    }

    return error;
  }

  libcommon_return_t _setup_gpu_buffers(SDLCtx *ctx) {
    uint32_t data_size = 0;

    data_size = static_cast<uint32_t>(sizeof(Vec3) * ctx->particle_count);

    SDL_GPUTransferBufferCreateInfo ti = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = data_size,
      .props = 0,
    };

    ctx->bufs.point_sprites.t = SDL_CreateGPUTransferBuffer(ctx->device, &ti);
    if (!ctx->bufs.point_sprites.t) {
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::TransferBufferAlloc,
      });
    }

    SDL_GPUBufferCreateInfo bi = {
      .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
      .size = data_size,
      .props = 0,
    };

    ctx->bufs.point_sprites.b = SDL_CreateGPUBuffer(ctx->device, &bi);
    if (!ctx->bufs.point_sprites.b) {
      SDL_ReleaseGPUTransferBuffer(ctx->device, ctx->bufs.point_sprites.t);
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::GpuBufferAlloc,
      });
    }

    return ctx;
  }

  SDLError _teardown_buffers(SDLError error) {
    if (error.type > SDLErrorType::TransferBufferAlloc ||
        error.type > SDLErrorType::GpuBufferAlloc) {
      if (error.ctx->bufs.point_sprites.b) {
        SDL_ReleaseGPUBuffer(error.ctx->device, error.ctx->bufs.point_sprites.b);
      }
      if (error.ctx->bufs.point_sprites.t) {
        SDL_ReleaseGPUTransferBuffer(error.ctx->device, error.ctx->bufs.point_sprites.t);
      }
    }

    return error;
  }

  SDLError _teardown_on_error(SDLError error) {
    _teardown_buffers(error);
    _release_window_and_destroy_gpu_device(error);
    _destroy_window(error);
    _quit_sdl(error);

    return error;
  }

  libcommon_return_t initialize_and_setup(uint32_t particle_count) {
    return _init_sdl(particle_count)
      .and_then(_create_window)
      .and_then(_create_gpu_device_and_claim_window)
      .and_then(_setup_gpu_buffers)
      .transform_error(_teardown_on_error);
  }

  void teardown(SDLCtx *ctx) {
    // Wrap the ctx in an SDLError to reuse the _teardown_on_error logic.
    SDLError err = { .ctx = ctx, .type = SDLErrorType::None };
    _teardown_on_error(err);
  }

  bool update(SDLCtx *ctx) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
        case SDL_EVENT_QUIT:
          return false;
          break;
      }
    }

    return true;
  }
}
