#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
//#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

constexpr int HEIGHT{720};
constexpr int WIDTH{(HEIGHT * 16) / 10};

SDL_Window *w{nullptr};
SDL_Renderer *r{nullptr};

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  if (!SDL_CreateWindowAndRenderer("Fluid2D", WIDTH, HEIGHT, 0, &w, &r)) {
    SDL_Log("Failed to create window + renderer: %s\n", SDL_GetError());
    exit(1);
  }

  // Support opacity.
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

  // Main Loop
  SDL_Event ev;
  bool should_quit = false;

  SDL_zero(ev);
  while (!should_quit) {
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_EVENT_QUIT:
        should_quit = true;
        break;
      }
    }
  }

  SDL_DestroyRenderer(r);
  SDL_DestroyWindow(w);
  SDL_Quit();

  return 0;
}
