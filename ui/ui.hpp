#ifndef UI_H
#define UI_H

#include <stdint.h>

#include <SDL3/SDL_render.h>

#include "microui.hpp"

constexpr mu_Real PCOUNT_MAX{10'000};
constexpr mu_Real PCOUNT_MIN{1};
constexpr mu_Real PCOUNT_STEP{1};
constexpr mu_Real TIMESTEP_MAX{120};      // Unit: fps
constexpr mu_Real TIMESTEP_MIN{10};       // Unit: fps
constexpr mu_Real TIMESTEP_STEP{10};      // Unit: fps, Increments of 10fps
constexpr mu_Real SIMSTEPS_MAX{5};
constexpr mu_Real SIMSTEPS_MIN{1};
constexpr mu_Real SIMSTEPS_STEP{1};
constexpr mu_Real GRAVITY_Y_MAX{20.0f};
constexpr mu_Real GRAVITY_Y_MIN{-20.0f};
constexpr mu_Real GRAVITY_Y_STEP{0.1f};

struct Ui {
  mu_Context *ctx;
  mu_Real pcount;     // Particle count
  mu_Real time_step;  // Amount to advance per sim step (in fps)
  mu_Real sim_steps;  // Sim steps per frame
  mu_Real gravity_y;
  mu_Real frame_time_sim;
  mu_Real frame_time_step;
  bool draw_ui;          // Toggle drawing of UI.

  Ui();
  ~Ui();

  void update_mouse_pos(int32_t x, int32_t y);
  void update_mouse_down(int32_t x, int32_t y);
  void update_mouse_up(int32_t x, int32_t y);
  void draw(SDL_Renderer *r);

private:
  char fmtbuf[16];

  void enqueue_draw_cmds();
  void execute_draw_cmds(SDL_Renderer *r);
};

#endif
