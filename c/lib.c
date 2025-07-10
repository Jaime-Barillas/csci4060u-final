#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


SDL_Window *window = NULL;
SDL_GPUDevice *device = NULL;
SDL_GPUGraphicsPipeline *pipeline = NULL;
SDL_GPUBuffer *vertex_buffer = NULL;
SDL_GPUBuffer *storage_buffer = NULL;

#define SCREEN_QUAD_VERTEX_COUNT (6)
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

/* UI State */
int width = 0;
int height = 0;
char run = 1;


struct Particle {
  float x;
  float y;
  float z;
  float _padding;
};

SDL_GPUBuffer * upload_buffer(SDL_GPUBufferUsageFlags, const void *, int);
SDL_GPUShader * load_shader(const char *, SDL_GPUShaderStage, unsigned int, unsigned int);
SDL_GPUGraphicsPipeline * create_pipeline(const char *);


/* Temporary Code */
#define VERTEX_COUNT (200)
struct Particle particles[VERTEX_COUNT];
void generate_particles(void) {
  for (int i = 0; i < VERTEX_COUNT; i++) {
    particles[i].x = (SDL_randf() * 2) - 1.0f;
    particles[i].y = (SDL_randf() * 2) - 1.0f;
    particles[i].z = (SDL_randf() * 2) - 1.0f;
  }
}


void log_info(char * msg, ...) {
  #ifdef LogEnabled
  static char fmt[256];
  int msg_len = 256 - 27 - strlen(msg);
  va_list args;
  va_start(args, msg);
  strncpy(fmt, "\x1b[1m[C]\x1b[0m \x1b[96mInfo\x1b[0m: ", 27);
  strncpy(&(fmt[27]), msg, msg_len > 0 ? msg_len : 0);
  vprintf(fmt, args);
  va_end(args);
  #endif
}

void log_warn(char * msg, ...) {
  #ifdef LogEnabled
  static char fmt[256];
  int msg_len = 256 - 27 - strlen(msg);
  va_list args;
  va_start(args, msg);
  strncpy(fmt, "\x1b[1m[C]\x1b[0m \x1b[96mWarn\x1b[0m: ", 27);
  strncpy(&(fmt[27]), msg, msg_len > 0 ? msg_len : 0);
  vprintf(fmt, args);
  va_end(args);
  #endif
}

void log_err(char * msg, ...) {
  #ifdef LogEnabled
  static char fmt[256];
  int msg_len = 256 - 28 - strlen(msg);
  va_list args;
  va_start(args, msg);
  strncpy(fmt, "\x1b[1m[C]\x1b[0m \x1b[96mError\x1b[0m: ", 28);
  strncpy(&(fmt[28]), msg, msg_len > 0 ? msg_len : 0);
  vprintf(fmt, args);
  va_end(args);
  #endif
}

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


  /* Temporary Code */
  generate_particles();
  vertex_buffer = upload_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, screen_quad, sizeof(screen_quad));
  storage_buffer = upload_buffer(SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ, particles, sizeof(particles));
  if (!vertex_buffer || !storage_buffer) {
    goto fail_claim;
  }


  pipeline = create_pipeline(exe_path);
  if (!pipeline) {
    log_err("Failed to create GPU graphics pipeline!\n");
    goto fail_pipeline;
  }


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

void render_ui(void) {
  SDL_GPUCommandBuffer *render_cmds;
  SDL_GPUTexture *tex;
  bool res;

  uniforms.sphere_count = VERTEX_COUNT;
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
    .buffer = vertex_buffer,
    .offset = 0
  };

  SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(render_cmds, &cti, 1, NULL);
  {
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUVertexBuffers(pass, 0, &screen_quad_binding, 1);
    SDL_BindGPUFragmentStorageBuffers(pass, 0, &storage_buffer, 1);
    SDL_PushGPUFragmentUniformData(render_cmds, 0, &uniforms, sizeof(uniforms));
    SDL_DrawGPUPrimitives(pass, SCREEN_QUAD_VERTEX_COUNT, 1, 0, 0);
  }
  SDL_EndGPURenderPass(pass);
  SDL_SubmitGPUCommandBuffer(render_cmds);
}

void destroy_ui(void) {
  SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
  SDL_ReleaseGPUBuffer(device, storage_buffer);
  SDL_ReleaseGPUBuffer(device, vertex_buffer);
  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyGPUDevice(device);
  SDL_DestroyWindow(window);
  SDL_Quit();
}


/**********************/
/* Internal Functions */
/**********************/

SDL_GPUBuffer * upload_buffer(SDL_GPUBufferUsageFlags buffer_usage, const void *data, int data_size) {
  SDL_GPUBuffer *buffer = NULL;
  SDL_GPUTransferBuffer *transfer_buffer;


  // 1. Allocate necessary buffers.
  SDL_GPUTransferBufferCreateInfo transfer_info = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = data_size,
    .props = 0
  };

  transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
  if (!transfer_buffer) {
    log_err("Failed to allocate GPU transfer buffer!\n");
    goto cleanup;
  }

  SDL_GPUBufferCreateInfo buf_info = {
    .usage = buffer_usage,
    .size = data_size,
    .props = 0
  };

  buffer = SDL_CreateGPUBuffer(device, &buf_info);
  if (!buffer) {
    log_err("Failed to allocate GPU buffer!\n");
    goto cleanup;
  }


  // 2. Copy data to GPU transfer buffer.
  SDL_GPUTransferBufferLocation source = {
    .transfer_buffer = transfer_buffer,
    .offset = 0
  };

  SDL_GPUBufferRegion destination = {
    .buffer = buffer,
    .offset = 0,
    .size = data_size
  };

  void *mapping = SDL_MapGPUTransferBuffer(device, transfer_buffer, true);
  if (!mapping) {
    log_err("Failed to map transfer buffer!\n");
    goto cleanup_buffer;
  }

  SDL_memcpy(mapping, data, data_size);

  SDL_UnmapGPUTransferBuffer(device, transfer_buffer);


  // 3. Record and submit copy pass.
  SDL_GPUCommandBuffer *copy_cmds = SDL_AcquireGPUCommandBuffer(device);
  if (!copy_cmds) {
    log_err("Failed to acquire a GPU command buffer to upload particles!\n");
    goto cleanup_buffer;
  }

  SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(copy_cmds);
  SDL_UploadToGPUBuffer(pass, &source, &destination, true);
  SDL_EndGPUCopyPass(pass);

  SDL_SubmitGPUCommandBuffer(copy_cmds);

  cleanup:
  if (transfer_buffer) { SDL_ReleaseGPUTransferBuffer(device, transfer_buffer); }
  return buffer;

  cleanup_buffer:
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
  SDL_ReleaseGPUBuffer(device, buffer);
  buffer = NULL;
  return buffer;
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
    .code = code,
    .code_size = code_size,
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
    goto fail_vertex;
  }

  SDL_zero(path_buf);
  snprintf(path_buf, 256, "%s/shaders/fragment.spv", exe_path);
  SDL_GPUShader *fragment_shader = load_shader(path_buf, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1);
  if (!fragment_shader) {
    log_err("Failed to create fragment shader!\n");
    goto fail_fragment;
  }


  SDL_GPUVertexBufferDescription vbuf_desc = {
    .slot = 0,
    .pitch = sizeof(screen_quad) / SCREEN_QUAD_VERTEX_COUNT,
    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    .instance_step_rate = 0
  };

  SDL_GPUVertexAttribute vbuf_attr = {
    .buffer_slot = 0,
    .location = 0,
    .offset = 0,
    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3
  };

  SDL_GPUColorTargetDescription pipeline_ctd = {
    .format = SDL_GetGPUSwapchainTextureFormat(device, window)
  };

  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
    .vertex_shader = vertex_shader,
    .fragment_shader = fragment_shader,
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .vertex_input_state = {
      .num_vertex_buffers = 1,
      .vertex_buffer_descriptions = &vbuf_desc,
      .num_vertex_attributes = 1,
      .vertex_attributes = &vbuf_attr
    },
    .target_info = {
      .num_color_targets = 1,
      .color_target_descriptions = &pipeline_ctd,
    }
  };

  graphics_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);



                 SDL_ReleaseGPUShader(device, fragment_shader);
  fail_fragment: SDL_ReleaseGPUShader(device, vertex_shader);

  fail_vertex:
  return graphics_pipeline;
}
