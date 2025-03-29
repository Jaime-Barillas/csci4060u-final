#include <stdint.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

#include "ui.hpp"

constexpr int HEIGHT{720};
constexpr int WIDTH{(HEIGHT * 16) / 10};

SDL_Window *w{nullptr};
SDL_Renderer *r{nullptr};

int main(int, char**) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Rect rect;
  SDL_DisplayID display = SDL_GetPrimaryDisplay();
  SDL_GetDisplayUsableBounds(display, &rect);
  int32_t window_height = (int32_t)SDL_floorf(rect.h / HEIGHT) * HEIGHT;
  int32_t window_width = (window_height * 16) / 10;
  if (!SDL_CreateWindowAndRenderer("CSCI4060U - Fluid2D", window_width, window_height, 0, &w, &r)) {
    SDL_Log("Failed to create window + renderer: %s\n", SDL_GetError());
    exit(1);
  }

  // Support opacity.
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_SetRenderLogicalPresentation(r, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

  // Main Loop
  Ui ui{};
  SDL_Event ev;
  bool should_quit = false;

  SDL_zero(ev);
  while (!should_quit) {
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_EVENT_QUIT) { should_quit = true; }

      SDL_ConvertEventToRenderCoordinates(r, &ev);
      switch (ev.type) {
      case SDL_EVENT_QUIT:
        should_quit = true;
        break;

      case SDL_EVENT_MOUSE_MOTION:
        ui.update_mouse_pos(ev.button.x, ev.button.y);
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        ui.update_mouse_down(ev.button.x, ev.button.y);
        break;

      case SDL_EVENT_MOUSE_BUTTON_UP:
        ui.update_mouse_up(ev.button.x, ev.button.y);
        break;
      }
    }

    SDL_SetRenderDrawColor(r, 48, 34, 24, 255);
    SDL_RenderClear(r);

    // TODO: Draw Particles.
    ui.draw(r);

    SDL_RenderPresent(r);
  }

  SDL_DestroyRenderer(r);
  SDL_DestroyWindow(w);
  SDL_Quit();

  return 0;
}
