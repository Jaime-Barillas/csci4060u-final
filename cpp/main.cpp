#include <stdint.h>
#include <stdlib.h>

#include <omp.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "ui.hpp"
#include "simulator.hpp"

constexpr int HEIGHT{720};
constexpr int WIDTH{(HEIGHT * 16) / 10};
constexpr int AVG_FPS_BUF_SIZE = 15;

SDL_Window *w{nullptr};
SDL_Renderer *r{nullptr};

int main(int, char**) {
  omp_set_num_threads(omp_get_num_procs());

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
  SDL_SetRenderVSync(r, 1);

  // Main Loop
  Ui ui{};
  SDL_Event ev;
  bool should_quit = false;
  uint64_t sim_time;
  float sim_times[AVG_FPS_BUF_SIZE];
  float step_times[AVG_FPS_BUF_SIZE];
  uint32_t sim_time_idx = 0;
  uint32_t step_time_idx = 0;
  float sim_time_total;
  float step_time_total;

  // NOTE: In SDL3, +y goes down so ui.gravity_y needs to be negated to match.
  Simulator sim(WIDTH, HEIGHT, ui.pcount, ui.time_step, ui.sim_steps, -ui.gravity_y);

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

    sim.set_time_step(ui.time_step);
    sim.set_sim_steps((int32_t)ui.sim_steps);
    sim.set_gravity_y(-ui.gravity_y);
    if (ui.pcount != sim.get_pcount()) {
      sim.reset_particles(ui.pcount);
    }

    // Perform simulation and track timings.
    sim_time = SDL_GetPerformanceCounter();
    step_times[step_time_idx] = sim.simulate();
    sim_time = SDL_GetPerformanceCounter() - sim_time;
    sim_times[sim_time_idx] = (float)sim_time / SDL_GetPerformanceFrequency();

    // Calculate average framerate over AVG_FPS_BUF_SIZE frames.
    step_time_idx = (step_time_idx + 1) % AVG_FPS_BUF_SIZE;
    sim_time_idx = (sim_time_idx + 1) % AVG_FPS_BUF_SIZE;
    sim_time_total = 0.0f;
    step_time_total = 0.0f;
    for (int32_t i = 0; i < AVG_FPS_BUF_SIZE; i++) {
      step_time_total += step_times[i];
      sim_time_total += sim_times[i];
    }
    ui.frame_time_step = step_time_total / AVG_FPS_BUF_SIZE;
    ui.frame_time_sim = sim_time_total / AVG_FPS_BUF_SIZE;

    // Draw particles and UI.
    sim.draw(r);
    ui.draw(r);

    SDL_RenderPresent(r);
  }

  SDL_DestroyRenderer(r);
  SDL_DestroyWindow(w);
  SDL_Quit();

  return 0;
}
