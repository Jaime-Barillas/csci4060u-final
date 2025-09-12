#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>
#include <stdio.h>
#include "log.h"


struct SDLBufferPair {
  SDL_GPUBuffer *buf;
  SDL_GPUTransferBuffer *tbuf;
  SDL_GPUBufferUsageFlags usage;
};

SDL_Window *window = NULL;
SDL_GPUDevice *device = NULL;
SDL_GPUGraphicsPipeline *pipeline = NULL;
struct SDLBufferPair vertex_buffer = {
  .buf = NULL,
  .tbuf = NULL,
  .usage = SDL_GPU_BUFFERUSAGE_VERTEX
};
struct SDLBufferPair storage_buffer = {
  .buf = NULL,
  .tbuf = NULL,
  .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
};

#define SCREEN_QUAD_VERTEX_COUNT 6
const float screen_quad[SCREEN_QUAD_VERTEX_COUNT * 3] = {
  -1.0f, -1.0f, 0.0f,
  -1.0f,  1.0f, 0.0f,
   1.0f,  1.0f, 0.0f,

   1.0f,  1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
  -1.0f, -1.0f, 0.0f,
};

struct Uniforms {
  unsigned int width;
  unsigned int height;
  int sphere_count;
  float sphere_radius;
} uniforms;

struct Vec3 {
  float x;
  float y;
  float z;
};

#define PARTICLE_COUNT 1000
// NOTE: std430 requires 16-byte alignment for vec3 so we pad the size calculation.
#define PARTICLE_BUFFER_SIZE ((sizeof(struct Vec3) + sizeof(float)) * PARTICLE_COUNT)
struct Particle {
  struct Vec3 pos;
  struct Vec3 vel;
  float density;
  float pressure;
};

/* UI State */
int width = 0;
int height = 0;
char run = 1;


bool allocate_buffers(struct SDLBufferPair *bufs, int data_size);
bool transfer_data(
  struct SDLBufferPair *bufs,
  const void *data,
  int data_size,
  bool cycle
);
bool transfer_particle_data(
  struct SDLBufferPair *bufs,
  const struct Particle *const *data,
  int data_count,
  bool cycle
);
bool upload_to_gpu(
  struct SDLBufferPair *bufs,
  int data_size,
  bool cycle
);
SDL_GPUShader * load_shader(
  const char *path,
  SDL_GPUShaderStage stage,
  unsigned int num_uniform_buffers,
  unsigned int num_storage_buffers);
SDL_GPUGraphicsPipeline * create_pipeline(const char *exe_path);


extern "C" {
char create_ui(const char *exe_path) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    log_err("Failed to initialize SDL!\n");
    goto fail_init;
  }

  /* Create Window */
  SDL_DisplayID display;
  SDL_Rect bounds;

  display = SDL_GetPrimaryDisplay();
  if (display == 0) {
    log_warn("Failed to get primary display handle! Using default window sizes.\n");
    bounds.w = 720;
    bounds.h = 720;
  } else {
    SDL_GetDisplayBounds(display, &bounds);
  }

  width = (bounds.w * 2) / 3;
  height = (bounds.h * 2) / 3;
  log_info("Window size: %dx%d\n", width, height);

  window = SDL_CreateWindow("fluid-sim-sph", width, height, 0);
  if (!window) {
    log_err("Failed to create window!\n");
    goto fail_window;
  }

  /* Create GPU Device */
  device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
  if (!device) {
    log_err("Failed to create GPU device!\n");
    goto fail_device;
  }

  if (!SDL_ClaimWindowForGPUDevice(device, window)) {
    log_err("Failed to claim window for GPU!\n");
    goto fail_claim;
  }
  log_info("GPU device created...\n");


  if (!allocate_buffers(&vertex_buffer, sizeof(screen_quad)) ||
      !transfer_data(&vertex_buffer, screen_quad, sizeof(screen_quad), false) ||
      !upload_to_gpu(&vertex_buffer, sizeof(screen_quad), false)) {
    log_err("Failed to upload screen quad to GPU!\n");
    goto fail_claim;
  }
  if (!allocate_buffers(&storage_buffer, PARTICLE_BUFFER_SIZE)) {
    log_err("Failed to allocate storage buffer!\n");
    goto fail_claim;
  }
  log_info("GPU buffers allocated...\n");


  pipeline = create_pipeline(exe_path);
  if (!pipeline) {
    log_err("Failed to create GPU graphics pipeline!\n");
    goto fail_pipeline;
  }
  log_info("GPU graphics pipeline...\n");


  return 1;

  fail_pipeline: SDL_ReleaseWindowFromGPUDevice(device, window);
  fail_claim:    SDL_DestroyGPUDevice(device);
  fail_device:   SDL_DestroyWindow(window);
  fail_window:   SDL_Quit();
  fail_init:
  return 0;
}

char update_ui(void) {
  SDL_Event ev;
  while (SDL_PollEvent(&ev)) {
    switch (ev.type) {
      case SDL_EVENT_QUIT:
        run = 0;
        break;
    }
  }

  // TODO: Return new UI state.
  return run;
}

void render_ui(const struct Particle *const *particles) {
  SDL_GPUCommandBuffer *render_cmds;
  SDL_GPUTexture *tex;
  bool res;

  if (!transfer_particle_data(&storage_buffer, particles, PARTICLE_COUNT, true) ||
      !upload_to_gpu(&storage_buffer, PARTICLE_BUFFER_SIZE, true)) {
    log_warn("Failed to upload updated particles!\n");
  }

  uniforms.sphere_count = PARTICLE_COUNT;
  uniforms.sphere_radius = 0.05f;

  render_cmds = SDL_AcquireGPUCommandBuffer(device);
  res = SDL_WaitAndAcquireGPUSwapchainTexture(
    render_cmds,
    window,
    &tex,
    &(uniforms.width),
    &(uniforms.height)
  );
  if (!res) {
    SDL_CancelGPUCommandBuffer(render_cmds);
    return;
  }

  SDL_GPUColorTargetInfo cti = {
    .texture = tex,
    .clear_color = { .r = 0.05, .g = 0.05, .b = 0.25 },
    .load_op = SDL_GPU_LOADOP_CLEAR,
    .store_op = SDL_GPU_STOREOP_STORE
  };

  SDL_GPUBufferBinding screen_quad_binding = {
    .buffer = vertex_buffer.buf,
    .offset = 0
  };

  SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(render_cmds, &cti, 1, NULL);
  {
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUVertexBuffers(pass, 0, &screen_quad_binding, 1);
    SDL_BindGPUFragmentStorageBuffers(pass, 0, &storage_buffer.buf, 1);
    SDL_PushGPUFragmentUniformData(render_cmds, 0, &uniforms, sizeof(uniforms));
    SDL_DrawGPUPrimitives(pass, SCREEN_QUAD_VERTEX_COUNT, 1, 0, 0);
  }
  SDL_EndGPURenderPass(pass);
  SDL_SubmitGPUCommandBuffer(render_cmds);
}

void destroy_ui(void) {
  SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
  SDL_ReleaseGPUBuffer(device, storage_buffer.buf);
  SDL_ReleaseGPUBuffer(device, vertex_buffer.buf);
  SDL_ReleaseGPUTransferBuffer(device, storage_buffer.tbuf);
  SDL_ReleaseGPUTransferBuffer(device, vertex_buffer.tbuf);
  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyGPUDevice(device);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
}


/**********************/
/* Internal Functions */
/**********************/

bool allocate_buffers(struct SDLBufferPair *bufs, int data_size) {
  if (!bufs->tbuf) {
    SDL_GPUTransferBufferCreateInfo transfer_info = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = (uint32_t)data_size,
      .props = 0
    };

    bufs->tbuf = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!bufs->tbuf) {
      log_err("Failed to allocate GPU transfer buffer!\n");
      return false;
    }
  }

  if (!bufs->buf) {
    SDL_GPUBufferCreateInfo buf_info = {
      .usage = bufs->usage,
      .size = (uint32_t)data_size,
      .props = 0
    };

    bufs->buf = SDL_CreateGPUBuffer(device, &buf_info);
    if (!bufs->buf) {
      log_err("Failed to allocate GPU buffer!\n");
      return false;
    }
  }

  return true;
}

bool transfer_data(
  struct SDLBufferPair *bufs,
  const void *data,
  int data_size,
  bool cycle
) {
  void *mapping = SDL_MapGPUTransferBuffer(device, bufs->tbuf, cycle);
  if (!mapping) {
    log_err("Failed to map transfer buffer!\n");
    return false;
  }

  SDL_memcpy(mapping, data, data_size);

  SDL_UnmapGPUTransferBuffer(device, bufs->tbuf);

  return true;
}

bool transfer_particle_data(
  struct SDLBufferPair *bufs,
  const struct Particle *const *data,
  int data_count,
  bool cycle
) {
  float *mapping = (float*)SDL_MapGPUTransferBuffer(device, bufs->tbuf, cycle);
  if (!mapping) {
    log_err("Failed to map transfer buffer!\n");
    return false;
  }

  // NOTE: To match glsl std430 layout we need to insert padding as every
  //       fourth element.
  for (int i = 0; i < data_count; i++) {
    mapping[(i * 4) + 0] = data[i]->pos.x;
    mapping[(i * 4) + 1] = data[i]->pos.y;
    mapping[(i * 4) + 2] = data[i]->pos.z;
    mapping[(i * 4) + 3] = 0;
  }

  SDL_UnmapGPUTransferBuffer(device, bufs->tbuf);

  return true;
}

bool upload_to_gpu(
  struct SDLBufferPair *bufs,
  int data_size,
  bool cycle
) {

  SDL_GPUCommandBuffer *copy_cmds = SDL_AcquireGPUCommandBuffer(device);
  if (!copy_cmds) {
    log_err("Failed to acquire a GPU command buffer to upload particles!\n");
    return false;
  }

  SDL_GPUTransferBufferLocation source = {
    .transfer_buffer = bufs->tbuf,
    .offset = 0
  };

  SDL_GPUBufferRegion destination = {
    .buffer = bufs->buf,
    .offset = 0,
    .size = (uint32_t)data_size
  };

  SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(copy_cmds);
  SDL_UploadToGPUBuffer(pass, &source, &destination, cycle);
  SDL_EndGPUCopyPass(pass);

  SDL_SubmitGPUCommandBuffer(copy_cmds);

  return true;
}

SDL_GPUShader * load_shader(const char *path,
                            SDL_GPUShaderStage stage,
                            unsigned int num_uniform_buffers,
                            unsigned int num_storage_buffers) {
  SDL_GPUShader *shader = NULL;

  size_t code_size;
  void *code = SDL_LoadFile(path, &code_size);
  if (!code) {
    log_err("Failed to load shader: %s!\n", path);
    return NULL;
  }

  SDL_GPUShaderCreateInfo shader_info = {
    .code_size = code_size,
    .code = (uint8_t*)code,
    .entrypoint = "main",
    .format = SDL_GPU_SHADERFORMAT_SPIRV,
    .stage = stage,
    .num_samplers = 0,
    .num_storage_textures = 0,
    .num_storage_buffers = num_storage_buffers,
    .num_uniform_buffers = num_uniform_buffers,
    .props = 0
  };

  shader = SDL_CreateGPUShader(device, &shader_info);
  SDL_free(code);

  return shader;
}

SDL_GPUGraphicsPipeline * create_pipeline(const char *exe_path) {
  SDL_GPUGraphicsPipeline *graphics_pipeline = NULL;
  char path_buf[256];


  SDL_zero(path_buf);
  snprintf(path_buf, 256, "%s/shaders/vertex.spv", exe_path);
  SDL_GPUShader *vertex_shader = load_shader(path_buf, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);
  if (!vertex_shader) {
    log_err("Failed to create vertex shader!\n");
    return graphics_pipeline;
  }

  SDL_zero(path_buf);
  snprintf(path_buf, 256, "%s/shaders/fragment.spv", exe_path);
  SDL_GPUShader *fragment_shader = load_shader(path_buf, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
  if (!fragment_shader) {
    log_err("Failed to create fragment shader!\n");
    SDL_ReleaseGPUShader(device, vertex_shader);
    return graphics_pipeline;
  }


  SDL_GPUVertexBufferDescription vbuf_desc = {
    .slot = 0,
    .pitch = sizeof(screen_quad) / SCREEN_QUAD_VERTEX_COUNT,
    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    .instance_step_rate = 0
  };

  SDL_GPUVertexAttribute vbuf_attr = {
    .location = 0,
    .buffer_slot = 0,
    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
    .offset = 0,
  };

  SDL_GPUColorTargetDescription pipeline_ctd = {
    .format = SDL_GetGPUSwapchainTextureFormat(device, window)
  };

  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
    .vertex_shader = vertex_shader,
    .fragment_shader = fragment_shader,
    .vertex_input_state = {
      .vertex_buffer_descriptions = &vbuf_desc,
      .num_vertex_buffers = 1,
      .vertex_attributes = &vbuf_attr,
      .num_vertex_attributes = 1,
    },
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .target_info = {
      .color_target_descriptions = &pipeline_ctd,
      .num_color_targets = 1,
    }
  };

  graphics_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);



                 SDL_ReleaseGPUShader(device, fragment_shader);
  fail_fragment: SDL_ReleaseGPUShader(device, vertex_shader);

  fail_vertex:
  return graphics_pipeline;
}
