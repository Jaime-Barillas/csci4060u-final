#include <stdbool.h>
#include <stdint.h>
#include <string.h> // memset().
#include <stdlib.h> // malloc()

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_events.h>
#include <cglm/struct.h>
#include <microui.h>
#include "sim.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define QUARTER_60FPS (1.0 / 60 / 4)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static bool draw_ui;
static mu_Context *ui;
static mu_Real ui_pcount;
static mu_Real ui_time_step;
static mu_Real ui_sim_steps;
static mu_Real ui_interaction_radius;
static mu_Real ui_gravity_x;
static mu_Real ui_gravity_y;
static int ui_draw_grid;

static SimCtx ctx;
static uint64_t old_time;

static int text_width(mu_Font _, const char *text, int len) {
    if (len == -1) { len = strlen(text); }
    return SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * len;
}

static int text_height(mu_Font _) {
    return SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
}

void update_sim_settings(SimCtx *ctx) {
    ctx->time_step = ui_time_step;
    ctx->sim_steps = (int64_t)ui_sim_steps;
    ctx->gravity.x = ui_gravity_x;
    ctx->gravity.y = ui_gravity_y;
    ctx->interaction_radius = (int64_t)ui_interaction_radius;
    ctx->cell_size = ctx->interaction_radius * 2;
    ctx->grid_width = (int64_t)SDL_ceil((float)ctx->window_width / ctx->cell_size);
    ctx->grid_height = (int64_t)SDL_ceil((float)ctx->window_height / ctx->cell_size);

    if (ctx->pcount != (int64_t)ui_pcount) {
        if (ctx->ps) { free(ctx->ps); }
        ctx->pcount = (int64_t)ui_pcount;
        ctx->ps = malloc(ctx->pcount * sizeof(Particle));
        init_particles(ctx);
    }
    if (ctx->cell_count != (ctx->grid_width * ctx->grid_height)) {
        if (ctx->cell_pcount) { free(ctx->cell_pcount); }
        ctx->cell_count = ctx->grid_width * ctx->grid_height;
        ctx->cell_pcount = malloc(ctx->cell_count * sizeof(int64_t));
    }
}

void initialize(void) {
    ui = malloc(sizeof(mu_Context));
    mu_init(ui);
    draw_ui = true;
    ui->text_width = text_width;
    ui->text_height = text_height;
    ui_time_step = QUARTER_60FPS * 2;
    ui_sim_steps = 1.0;
    ui_interaction_radius = 30;
    ui_gravity_x = 0.0;
    ui_gravity_y = 0.0;
    ui_pcount = 100;
    ui_draw_grid = 1;

    ctx.window_width = WINDOW_WIDTH;
    ctx.window_height = WINDOW_HEIGHT;
    update_sim_settings(&ctx);

    init_particles(&ctx);
    old_time = SDL_GetTicksNS();
}

void draw(SimCtx *ctx) {
    SDL_SetRenderDrawColor(renderer, 96, 61, 31, 255);
    SDL_RenderClear(renderer);
    SDL_FRect rect;

    // Render grid.
    if (ui_draw_grid != 0) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 16);
        rect.y = 0;
        rect.w = 2;
        rect.h = WINDOW_HEIGHT;
        for (size_t x = 0; x < WINDOW_WIDTH; x += ctx->cell_size) {
            rect.x = x;
            SDL_RenderFillRect(renderer, &rect);
        }
        rect.x = 0;
        rect.w = WINDOW_WIDTH;
        rect.h = 2;
        for (size_t y = 0; y < WINDOW_HEIGHT; y += ctx->cell_size) {
            rect.y = y;
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // Render the particles
    SDL_SetRenderDrawColor(renderer, 55, 75, 178, 255);
    rect.w = 10;
    rect.h = 10;
    for (int i = 0; i < ctx->pcount; i++) {
        rect.x = ctx->ps[i].pos.x - (rect.w / 2.0);
        rect.y = ctx->ps[i].pos.y - (rect.h / 2.0);
        SDL_RenderFillRect(renderer, &rect);
    }

    // Render UI
    if (draw_ui) {
        mu_begin(ui);
        if (mu_begin_window_ex(ui, "Settings", mu_rect(10, 10, 0, 0), MU_OPT_AUTOSIZE | MU_OPT_NOCLOSE | MU_OPT_NORESIZE)) {
            mu_layout_row(ui, 1, (int[]){365}, 0);
            mu_label(ui, "Press 'Space Bar' to show/hide window");

            mu_layout_row(ui, 4, (int[]){155, 50, 102, 50}, 0);
            mu_label(ui, "Particle Count:");
            if (mu_button(ui, "-100")) {
                ui_pcount = SDL_max(ui_pcount - 100, 1);
            }
            if (mu_number_ex(ui, &ui_pcount, 0.2, "%.0f", MU_OPT_ALIGNCENTER)) {
                ui_pcount = SDL_clamp(ui_pcount, 1, 3000);
            }
            if (mu_button(ui, "+100")) {
                ui_pcount = SDL_min(ui_pcount + 100, 3000);
            }

            mu_layout_row(ui, 2, (int[]){155, 210}, 0);
            mu_label(ui, "Time step:");
            mu_slider_ex(ui, &ui_time_step, QUARTER_60FPS, QUARTER_60FPS * 8, QUARTER_60FPS, "%.3fs", MU_OPT_ALIGNCENTER);
            mu_label(ui, "Simulation Steps:");
            mu_slider_ex(ui, &ui_sim_steps, 1.0, 5.0, 1.0, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ui, "Interaction Radius:");
            mu_slider_ex(ui, &ui_interaction_radius, 1.0, ctx->window_width * 0.25, 1.0, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ui, "Gravity X:");
            mu_slider_ex(ui, &ui_gravity_x, -10.0, 10.0, 0.1, "%.1f", MU_OPT_ALIGNCENTER);
            mu_label(ui, "Gravity Y:");
            mu_slider_ex(ui, &ui_gravity_y, -10.0, 10.0, 0.1, "%.1f", MU_OPT_ALIGNCENTER);
            mu_label(ui, ""); mu_label(ui, "");
            mu_layout_row(ui, 1, (int[]){365}, 0);
            mu_checkbox(ui, "Show Grid", &ui_draw_grid);
            mu_end_window(ui);
        }
        mu_end(ui);

        mu_Command *cmd = NULL;
        mu_Color color;
        while (mu_next_command(ui, &cmd)) {
            switch (cmd->type) {
                case MU_COMMAND_TEXT:
                    color = cmd->text.color;
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                    SDL_RenderDebugText(renderer, cmd->text.pos.x, cmd->text.pos.y, cmd->text.str);
                    break;
                case MU_COMMAND_RECT:
                    color = cmd->rect.color;
                    rect.x = cmd->rect.rect.x;
                    rect.y = cmd->rect.rect.y;
                    rect.w = cmd->rect.rect.w;
                    rect.h = cmd->rect.rect.h;
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                    SDL_RenderFillRect(renderer, &rect);
                    break;
                case MU_COMMAND_ICON:
                    color = cmd->icon.color;
                    int x = cmd->icon.rect.x + (cmd->icon.rect.w - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;
                    int y = cmd->icon.rect.y + (cmd->icon.rect.h - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;
                    if (cmd->icon.id == MU_ICON_CHECK) {
                        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                        SDL_RenderDebugText(renderer, x, y, "x");
                    }
                    break;
                case MU_COMMAND_CLIP:
                    break;
                default:
                    break;
            }
        }
    }

    // TODO: Move to iterate function?
    update_sim_settings(ctx);
    SDL_RenderPresent(renderer);
}



//***************
//* SDL Functions
//***************

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Log("\x1b[2J"); // Clear screen.
    SDL_SetAppMetadata("CSCI40460U-FINAL", "1.0", "net.ontariotechu.jaime.barillas.CSCI4060U-FINAL");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("CSCI4060U - Final", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    initialize();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *_, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.scancode) {
            case SDL_SCANCODE_SPACE:
                draw_ui = !draw_ui;
            default:
                break;
        }
    }
    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION:
            mu_input_mousemove(ui, event->motion.x, event->motion.y);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            mu_input_mousedown(ui, event->button.x, event->button.y, MU_MOUSE_LEFT);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            mu_input_mouseup(ui, event->button.x, event->button.y, MU_MOUSE_LEFT);
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *_)
{
    uint64_t new_time = SDL_GetTicksNS();
    uint64_t dt = new_time - old_time;
    old_time = new_time;
    SDL_Log("\x1b[H\x1b[0KFPS: %.2f", 1.0e9 / dt);
    SDL_Log("\x1b[0KLast Frame Time: %.2fms", dt / 1.0e6);

    for (int64_t i = 0; i < ctx.sim_steps; i++) {
        sim_step(&ctx, ctx.time_step);
    }
    draw(&ctx);

    // if ((dt / 1.0e6) < MILLI_PER_FRAME)
    // {
    //     SDL_DelayNS(MILLI_PER_FRAME - (dt / 1.0e6));
    // }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}
