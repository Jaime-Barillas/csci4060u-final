// C API for Pony

#include "lib.h"
#include "SDL3/SDL_gpu.h"
#include "Vec3.h"
#include <cstdint>
#include <map>


typedef uint32_t CError;
typedef const Vec3 *const * vec3_list;

const std::map<libcommon::SDLErrorType, CError> sdl_err_map = {
  { libcommon::SDLErrorType::Initialization,    1 << 0 },
  { libcommon::SDLErrorType::WindowCreation,    1 << 1 },
  { libcommon::SDLErrorType::GpuDeviceCreation, 1 << 2 },
  { libcommon::SDLErrorType::ClaimingWindow,    1 << 3 },
};

bool copy_particles(SDL_GPUTransferBuffer *tbuf, const void *particles_obj) {
  vec3_list particles = static_cast<vec3_list>(particles_obj);
  return true;
}

extern "C" {

  CError c_make_ui(libcommon::SDLCtx **ctx) {
    auto res = libcommon::make_window();
    if (res) {
      *ctx = res.value();
      return 0;
    } else {
      return sdl_err_map.at(res.error().type);
    }
  }

  void c_destroy_ui(void *ctx_ptr) {
    if (!ctx_ptr) { return; }
    libcommon::SDLCtx *ctx = static_cast<libcommon::SDLCtx*>(ctx_ptr);
    libcommon::destroy_window(ctx);
  }

  uint8_t c_update_ui(void *ctx_ptr) {
    if (!ctx_ptr) { return 0; }
    libcommon::SDLCtx *ctx = static_cast<libcommon::SDLCtx*>(ctx_ptr);
    return libcommon::update_window(ctx) ? 1 : 0;
  }

  void c_draw_ui(void *ctx_ptr, vec3_list positions) {
    if (!ctx_ptr) { return; }
    libcommon::SDLCtx *ctx = static_cast<libcommon::SDLCtx*>(ctx_ptr);
    auto a = libcommon::draw_particles(ctx, copy_particles, static_cast<const void*>(positions));
  }
};
