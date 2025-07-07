#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


SDL_Window *window = NULL;
SDL_GPUDevice *device = NULL;

/* UI State */
int width = 0;
int height = 0;
char run = 1;


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

char create_ui(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    log_err("Failed to initialize SDL!\n");
    return 0;
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
    SDL_Quit();
    return 0;
  }

  /* Create GPU Device */
  device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
  if (!device) {
    log_err("Failed to create GPU device!");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  if (!SDL_ClaimWindowForGPUDevice(device, window)) {
    log_err("Failed to claim window for GPU!");
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  return 1;
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
  SDL_GPUCommandBuffer *buf;
  SDL_GPUTexture *tex;
  unsigned int tex_width;
  unsigned int tex_height;
  bool res;

  buf = SDL_AcquireGPUCommandBuffer(device);
  res = SDL_WaitAndAcquireGPUSwapchainTexture(buf, window, &tex, &tex_width, &tex_height);
  if (!res) {
    SDL_CancelGPUCommandBuffer(buf);
    return;
  }

  SDL_GPUColorTargetInfo cti;
  cti.texture = tex;
  cti.clear_color.r = 0.05;
  cti.clear_color.g = 0.05;
  cti.clear_color.b = 0.25;
  cti.load_op = SDL_GPU_LOADOP_CLEAR;
  cti.store_op = SDL_GPU_STOREOP_STORE;

  SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(buf, &cti, 1, NULL);
  SDL_EndGPURenderPass(pass);
  SDL_SubmitGPUCommandBuffer(buf);
}

void destroy_ui(void) {
  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyGPUDevice(device);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
