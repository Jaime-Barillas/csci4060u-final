// C API for Pony

#include "lib.h"
#include "SDL3/SDL_gpu.h"
#include "Vec3.h"
#include <cstdint>
#include <utility>


typedef uint32_t CError;
typedef const Vec3 *const * vec3_list;

bool copy_particles(libcommon::SDLCtx *ctx, SDL_GPUTransferBuffer *tbuf, const void *particles_obj) {
  vec3_list particles = static_cast<vec3_list>(particles_obj);
  return true;
}

extern "C" {

  CError c_initialize_and_setup(const char *exe_dir, uint32_t particle_count, libcommon::SDLCtx **ctx) {
    auto res = libcommon::initialize_and_setup(exe_dir, particle_count);
    if (res) {
      *ctx = res.value();
      return 0;
    } else {
      return std::to_underlying(res.error().type);
    }
  }

  void c_teardown(void *ctx_ptr) {
    if (!ctx_ptr) { return; }
    libcommon::SDLCtx *ctx = static_cast<libcommon::SDLCtx*>(ctx_ptr);
    libcommon::teardown(ctx);
  }

  uint8_t c_update(void *ctx_ptr) {
    if (!ctx_ptr) { return 0; }
    libcommon::SDLCtx *ctx = static_cast<libcommon::SDLCtx*>(ctx_ptr);
    return libcommon::update(ctx) ? 1 : 0;
  }

  void c_draw(void *ctx_ptr, vec3_list positions) {
    if (!ctx_ptr) { return; }
    libcommon::SDLCtx *ctx = static_cast<libcommon::SDLCtx*>(ctx_ptr);
    libcommon::draw(ctx, copy_particles, static_cast<const void*>(positions));
  }
};
