#include "sdl3.h"
#include "log.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

namespace sdl3 {
  using sdl3_return_t = std::expected<SDLCtx, SDLError>;

  const char *_fn_ = "XXX";

  sdl3_return_t _init_sdl() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      return std::unexpected((SDLError){
        .ctx = {},
        .type = SDLErrorType::Initialization,
      });
    }

    return (SDLCtx){};
  }

  sdl3_return_t _create_window(SDLCtx ctx) {
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

    log_info("(%s) Window size: %dx%d\n", _fn_, bounds.w, bounds.h);

    ctx.window = SDL_CreateWindow("fluid-sim-sph", bounds.w, bounds.h, 0);
    if (!ctx.window) {
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::WindowCreation,
      });
    }

    return ctx;
  }

  sdl3_return_t _create_gpu_device_and_claim_window(SDLCtx ctx) {
    ctx.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr);
    if (!ctx.device) {
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::GpuDeviceCreation,
      });
    }

    if (!SDL_ClaimWindowForGPUDevice(ctx.device, ctx.window)) {
      return std::unexpected((SDLError){
        .ctx = ctx,
        .type = SDLErrorType::ClaimingWindow,
      });
    }

    return ctx;
  }

  SDLError _teardown_on_error(SDLError error) {
    switch (error.type) {
      case SDLErrorType::ClaimingWindow:
        SDL_DestroyGPUDevice(error.ctx.device);
        [[fallthrough]];
      case SDLErrorType::GpuDeviceCreation:
        SDL_DestroyWindow(error.ctx.window);
        [[fallthrough]];
      case SDLErrorType::WindowCreation:
        SDL_Quit();
        [[fallthrough]];
      case SDLErrorType::Initialization:
        break;
    }

    return error;
  }

  std::expected<SDLCtx, SDLError> make_window() {
    _fn_ = "sdl3::make_window";
    return _init_sdl()
      .and_then(_create_window)
      .and_then(_create_gpu_device_and_claim_window)
      .transform_error(_teardown_on_error);
  }
}
