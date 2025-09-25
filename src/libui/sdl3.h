#pragma once

#include <cstdint>
#include <expected>
#include <format>
#include <map>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>


namespace sdl3 {
  enum class SDLErrorType : int32_t {
    Initialization      = 1 << 0,
    WindowCreation      = 1 << 1,
    GpuDeviceCreation   = 1 << 2,
    ClaimingWindow      = 1 << 3,
    ShaderNotFound      = 1 << 4,
    ShaderCompilation   = 1 << 5,
    PipelineCompilation = 1 << 6,
  };

  // TODO: std::variant???
  union _SDLPipelinePtrTypes {
    SDL_GPUGraphicsPipeline *as_graphics;
    SDL_GPUComputePipeline *as_compute;
  };

  struct SDLCtx {
    SDL_Window *window;
    SDL_GPUDevice *device;
    std::map<std::string, _SDLPipelinePtrTypes> pipelines;
  };

  struct SDLError {
    SDLCtx ctx;
    SDLErrorType type;
  };

  enum class SDLShaderType : int32_t {
    Vertex = SDL_GPU_SHADERSTAGE_VERTEX,
    Fragment = SDL_GPU_SHADERSTAGE_FRAGMENT,
    Compute,
  };

  struct SDLShaderCreateInfoGraphics {
    uint32_t num_samplers;
    uint32_t num_storage_textures;
    uint32_t num_storage_buffers;
    uint32_t num_uniform_buffers;
  };

  struct SDLShaderCreateInfoCompute {
    uint32_t num_samplers;
    uint32_t num_readonly_storage_textures;
    uint32_t num_readonly_storage_buffers;
    uint32_t num_readwrite_storage_textures;
    uint32_t num_readwrite_storage_buffers;
    uint32_t num_uniform_buffers;
    uint32_t threadcount_x;
    uint32_t threadcount_y;
    uint32_t threadcount_z;
  };

  struct SDLShader {
    std::string path;
    std::string entry_point;
    SDLShaderType type;
    union {
      SDLShaderCreateInfoGraphics graphics_info;
      SDLShaderCreateInfoCompute compute_info;
    };
  };

  enum class SDLPipelineType : int32_t {
    Graphics,
    Compute,
  };

  struct SDLPipeline {
    std::string name;
    SDLPipelineType type;
    std::vector<SDLShader> shaders;
  };

  /**
   * Initialize SDL, create a window and gpu device, and claim the window for
   * the gpu.
   */
  std::expected<SDLCtx, SDLError> make_window();

  /**
   * TODO Documentation
   */
  std::expected<SDLCtx, SDLError> compile_pipeline(SDLCtx *ctx, SDLPipeline pipeline);
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
