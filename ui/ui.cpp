#include <stdio.h>
#include <string.h>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_rect.h>

#include "microui.hpp"
#include "ui.hpp"

static int text_width(mu_Font, const char *text, int len) {
  if (len < 0) { len = strlen(text); }
  return SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * len;
}

static int text_height(mu_Font) {
  return SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
}

Ui::Ui() : ctx{new mu_Context},
           pcount{400},
           time_step{TIMESTEP_STEP * 12},
           sim_steps{1},
           gravity_y{-9.80},
           draw_ui{true} {
  mu_init(ctx);
  ctx->text_width = text_width;
  ctx->text_height = text_height;
}

Ui::~Ui() {
  delete ctx;
}

void Ui::update_mouse_pos(int32_t x, int32_t y) {
  mu_input_mousemove(ctx, x, y);
}

void Ui::update_mouse_down(int32_t x, int32_t y) {
  mu_input_mousedown(ctx, x, y, MU_MOUSE_LEFT);
}

void Ui::update_mouse_up(int32_t x, int32_t y) {
  mu_input_mouseup(ctx, x, y, MU_MOUSE_LEFT);
}

void Ui::enqueue_draw_cmds() {
  if (!draw_ui) { return; }

  mu_begin(ctx);

  if (mu_begin_window_ex(
        ctx,
        "Settings",
        mu_rect(10, 10, 0, 0),
        MU_OPT_AUTOSIZE | MU_OPT_NOCLOSE | MU_OPT_NORESIZE
      )) {
    int info_row[]{365};
    mu_layout_row(ctx, 1, info_row, 0);
    mu_label(ctx, "Press 'Space Bar' to show/hide window");

    int pcount_row[]{155, 50, 102, 50};
    mu_layout_row(ctx, 4, pcount_row, 0);
    mu_label(ctx, "Particle Count:");
    if (mu_button(ctx, "-100")) {
      pcount = SDL_max(pcount - 100, PCOUNT_MIN);
    }
    if (mu_number_ex(ctx, &pcount, PCOUNT_STEP, "%.0f", MU_OPT_ALIGNCENTER)) {
      pcount = SDL_clamp(pcount, PCOUNT_MIN, PCOUNT_MAX);
    }
    if (mu_button(ctx, "+100")) {
      pcount = SDL_min(pcount + 100, PCOUNT_MAX);
    }

    int settings_row[]{155, 210};
    mu_layout_row(ctx, 2, settings_row, 0);
    mu_label(ctx, "Time step:");
    mu_slider_ex(ctx, &time_step, TIMESTEP_MIN, TIMESTEP_MAX, TIMESTEP_STEP, "%.0f(fps)", MU_OPT_ALIGNCENTER);
    mu_label(ctx, "Simulation Steps:");
    mu_slider_ex(ctx, &sim_steps, SIMSTEPS_MIN, SIMSTEPS_MAX, SIMSTEPS_STEP, "%.0f", MU_OPT_ALIGNCENTER);
    mu_label(ctx, "Gravity Y:");
    mu_slider_ex(ctx, &gravity_y, GRAVITY_Y_MIN, GRAVITY_Y_MAX, GRAVITY_Y_STEP, "%.1f", MU_OPT_ALIGNCENTER);

    int statistics_row[]{155, 210};
    mu_layout_row(ctx, 2, statistics_row, 0);
    snprintf(fmtbuf, sizeof(fmtbuf), "%.0f(%.4f)", 1.0f / frame_time, frame_time);
    mu_label(ctx, "FPS:");
    mu_label(ctx, fmtbuf);

    mu_end_window(ctx);
  }

  mu_end(ctx);
}

void Ui::execute_draw_cmds(SDL_Renderer *r) {
  mu_Command *cmd = nullptr;
  mu_Color color;
  SDL_FRect rect;

  while (mu_next_command(ctx, &cmd)) {
    switch (cmd->type) {
      case MU_COMMAND_TEXT:
        color = cmd->text.color;
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        SDL_RenderDebugText(r, cmd->text.pos.x, cmd->text.pos.y, cmd->text.str);
        break;

      case MU_COMMAND_RECT:
        color = cmd->rect.color;
        rect.x = cmd->rect.rect.x;
        rect.y = cmd->rect.rect.y;
        rect.w = cmd->rect.rect.w;
        rect.h = cmd->rect.rect.h;
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(r, &rect);
        break;

      case MU_COMMAND_ICON:
        color = cmd->icon.color;
        rect.x = cmd->icon.rect.x + (cmd->icon.rect.w - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
        rect.y = cmd->icon.rect.y + (cmd->icon.rect.h - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f;
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        if (cmd->icon.id == MU_ICON_CHECK) {
          SDL_RenderDebugText(r, rect.x, rect.y, "x");
        }
        break;

      default:
        break;
    }
  }
}

void Ui::draw(SDL_Renderer *r) {
  enqueue_draw_cmds();
  execute_draw_cmds(r);
}
