#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <map>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>


namespace libcommon {
  enum class SDLErrorType : uint32_t {
    BadParticleCount    = 1 << 0,
    Initialization      = 1 << 1,
    WindowCreation      = 1 << 2,
    GpuDeviceCreation   = 1 << 3,
    ClaimingWindow      = 1 << 4,
    TransferBufferAlloc = 1 << 5,
    GpuBufferAlloc      = 1 << 6,

    // The 'None' error allows sharing the teardown logic when a setup error
    // ocurrs and during regular shutdown.
    None                = 0xffff'ffff,
  };

  struct SDLCtx {
    SDL_Window *window = nullptr;
    SDL_GPUDevice *device = nullptr;
    struct {
      struct {
        SDL_GPUTransferBuffer *t = nullptr;
        SDL_GPUBuffer *b = nullptr;
      } point_sprites;
    } bufs;

    uint32_t particle_count = 0;
  };

  struct SDLError {
    SDLCtx *ctx;
    SDLErrorType type;
  };

  /**
   * Initialize and perform a complete setup.
   *
   * @returns The SDL context (window, gpu device, buffers, shaders &
   *          pipelines) or an error.
   */
  std::expected<SDLCtx*, SDLError> initialize_and_setup(uint32_t particle_count);

  /**
   * Teardown the SDL context.
   */
  void teardown(SDLCtx *ctx);

  /**
   * Poll SDL3 events.
   */
  bool update(SDLCtx *ctx);

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
  std::expected<SDLCtx*, SDLError> draw(
    SDLCtx *ctx,
    bool (*copy_callback)(SDL_GPUTransferBuffer *tbuf, const void *particles_obj),
    const void *particles_obj
  );
}

const std::map<libcommon::SDLErrorType, const char*> _err_message = {
  { libcommon::SDLErrorType::BadParticleCount,    "Invalid particle count" },
  { libcommon::SDLErrorType::Initialization,      "Failed to init SDL" },
  { libcommon::SDLErrorType::WindowCreation,      "Failed to create window" },
  { libcommon::SDLErrorType::GpuDeviceCreation,   "Failed to create gpu device" },
  { libcommon::SDLErrorType::ClaimingWindow,      "Failed to claim window for gpu" },
  { libcommon::SDLErrorType::TransferBufferAlloc, "Failed to allocate a transfer buffer" },
  { libcommon::SDLErrorType::GpuBufferAlloc,      "Failed to allocate a gpu buffer" },

  { libcommon::SDLErrorType::None,                "No error!" },
};

template<>
struct std::formatter<libcommon::SDLError> {
  constexpr auto parse(auto& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      throw std::format_error("Invalid format specifier");
    }
    return it;
  }

  auto format(const libcommon::SDLError &error, std::format_context &ctx) const {
    // TODO: Neater.
    libcommon::SDLCtx *c = error.ctx;
    const void *window = c ? static_cast<const void*>(c->window) : nullptr;
    const void *device = c ? static_cast<const void*>(c->device) : nullptr;
    uint32_t particle_count = c ? c->particle_count : 0;
    const void *point_sprites_t = c ? static_cast<const void*>(c->bufs.point_sprites.t) : nullptr;
    const void *point_sprites_b = c ? static_cast<const void*>(c->bufs.point_sprites.b) : nullptr;

    return std::format_to(
      ctx.out(),
R"({}
  window: {}  gpu_device: {}
  particle_count: {}
  bufs>point_sprits
    tbuf: {}  buf: {})",
      _err_message.at(error.type),
      window, device,
      particle_count,
      point_sprites_t, point_sprites_b
    );
  }
};
