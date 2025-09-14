#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>


namespace sdl3 {
  enum class SDLErrorType : int32_t {
    Initialization    = 1 << 0,
    WindowCreation    = 1 << 1,
    GpuDeviceCreation = 1 << 2,
    ClaimingWindow    = 1 << 3,
  };

  struct SDLCtx {
    SDL_Window *window;
    SDL_GPUDevice *device;
  };

  struct SDLError {
    SDLCtx ctx;
    SDLErrorType type;
  };

  /**
   * Initialize SDL, create a window and gpu device, and claim the window for
   * the gpu.
   */
  std::expected<SDLCtx, SDLError> make_window();
}

template<>
struct std::formatter<sdl3::SDLError> {
  constexpr auto parse(auto& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw std::format_error("Invalid format specifier");
    }
    return it;
  }

  auto format(const sdl3::SDLError &error, std::format_context &ctx) {
    return std::format_to(ctx.out(), "SDLError: {}", "TODO");
  }
};
