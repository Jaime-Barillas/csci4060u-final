#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>


namespace libcommon {
  enum class SDLErrorType : uint32_t {
    Initialization      = 1 << 0,
    WindowCreation      = 1 << 1,
    GpuDeviceCreation   = 1 << 2,
    ClaimingWindow      = 1 << 3,
    TransferBufferAlloc = 1 << 4,
    GpuBufferAlloc      = 1 << 5,
  };

  struct SDLCtx {
    SDL_Window *window;
    SDL_GPUDevice *device;
    struct {
      struct {
        SDL_GPUTransferBuffer *t;
        SDL_GPUBuffer *b;
      } point_sprites;
    } bufs;
  };

  struct SDLError {
    SDLCtx *ctx;
    SDLErrorType type;
  };

  /**
   * Initialize SDL, create a window and gpu device, and claim the window for
   * the gpu.
   */
  std::expected<SDLCtx*, SDLError> make_window();

  /**
   * Release the window from the gpu device, destroy the gpu device and
   * window, and deinit SDL.
   */
  void destroy_window(SDLCtx *ctx);

  /**
   * Allocate all gpu/transfer buffer pairs needed.
   */
  std::expected<SDLCtx*, SDLError> setup_gpu_buffers(SDLCtx *ctx, uint32_t particle_count);

  /**
   * Poll SDL3 events.
   */
  bool update_window(SDLCtx *ctx);

  /**
   * Draw the particles.
   *
   * @param copy_callback A function that returns true if it successfully
   *        copies the given particle list object to the provided SDL gpu
   *        transfer buffer.
   * @param particles_obj An object containing all the particles to copy.
   */
   // NOTE: C style callback allows the definition to be in the corresponding
   //       .cpp file instead of here in the header (or another header file)
   //       as would be necessary if it were a templated function. We only
   //       lose the ability to use C++ lambdas ¯\_(ツ)_/¯.
  std::expected<SDLCtx*, SDLError> draw_particles(
    SDLCtx *ctx,
    bool (*copy_callback)(SDL_GPUTransferBuffer *tbuf, const void *particles_obj),
    const void *particles_obj
  );
}

template<>
struct std::formatter<libcommon::SDLError> {
  constexpr auto parse(auto& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw std::format_error("Invalid format specifier");
    }
    return it;
  }

  auto format(const libcommon::SDLError &error, std::format_context &ctx) {
    return std::format_to(ctx.out(), "SDLError: {}", "TODO");
  }
};
