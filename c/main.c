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

#include <stdbool.h>
#include <stdint.h>
#include <string.h> // memset().

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TARGET_FPS 60
#define MILLI_PER_FRAME ( 1000.0 / TARGET_FPS )

#define PCOUNT 1000

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

typedef struct Particle_T {
    double vx;
    double vy;
    double x;
    double y;
} Particle;

Particle particles[PCOUNT];
uint64_t old_time;

double interaction_radius;
double bin_size;
bool draw_grid;
double collision_friction = 1;

void initialize(void) {
    SDL_Log("\x1b[2J"); // Clear screen.

    for (int i = 0; i < PCOUNT; i++) {
        particles[i].x = SDL_randf() * WINDOW_WIDTH;
        particles[i].y = SDL_randf() * WINDOW_HEIGHT;
        particles[i].vx = SDL_randf() * 20.0 - 10.0;
        particles[i].vy = SDL_randf() * 20.0 - 10.0;
    }

    old_time = SDL_GetTicksNS();
    interaction_radius = 20.0;
    bin_size = (interaction_radius * 2.0);
    draw_grid = true;
}

void draw(void) {
    // Render the particles
    SDL_SetRenderDrawColor(renderer, 96, 61, 31, 255);
    SDL_RenderClear(renderer);
    SDL_FRect rect;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 16);
    rect.y = 0;
    rect.w = 2;
    rect.h = WINDOW_HEIGHT;
    for (int x = 0; x < WINDOW_WIDTH; x += bin_size) {
        rect.x = x;
        SDL_RenderFillRect(renderer, &rect);
    }
    rect.x = 0;
    rect.w = WINDOW_WIDTH;
    rect.h = 2;
    for (int y = 0; y < WINDOW_HEIGHT; y += bin_size) {
        rect.y = y;
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_SetRenderDrawColor(renderer, 55, 75, 178, 255);
    rect.w = 5;
    rect.h = 5;
    for (int i = 0; i < PCOUNT; i++) {
        rect.x = particles[i].x - (rect.w / 2.0);
        rect.y = particles[i].y - (rect.h / 2.0);
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_RenderPresent(renderer);
}

size_t bin_index(int32_t x, int32_t y, int32_t width, int32_t height) {
    // FIXME: Handle negative x & y values, wrap around max index: (width * height).
    int32_t b_size = (int32_t)bin_size;
    return ((width * (y / b_size)) + (x / b_size)); // % (width * height);
}

void sort_into_bins(Particle ps[PCOUNT]) {
    // Counting + Cycle sort. Sorts particles based on the grid cell (bin) they
    // belong to using its index as their key.
    int32_t grid_width = (int32_t)SDL_ceil(WINDOW_WIDTH / bin_size);
    int32_t grid_height = (int32_t)SDL_ceil(WINDOW_HEIGHT / bin_size);
    size_t num_cells = (grid_width * grid_height);
    size_t count[num_cells];
    memset(count, 0, num_cells * sizeof(size_t));

    for (size_t i = 0; i < PCOUNT; i++) {
        int32_t x = (int32_t)ps[i].x;
        int32_t y = (int32_t)ps[i].y;
        size_t key = bin_index(x, y, grid_width, grid_height);
        count[key] += 1;
    }

    for (size_t i = 1; i < num_cells; i++) {
        count[i] += count[i - 1];
    }

    // In-place cycle sort.
    // See: https://stackoverflow.com/a/15719967
    // See: https://stackoverflow.com/a/62598539
    Particle temp;
    for (size_t i = 0; i < PCOUNT; i++) {
        int32_t x = (int32_t)ps[i].x;
        int32_t y = (int32_t)ps[i].y;
        size_t key = bin_index(x, y, grid_width, grid_height);
        while (i < count[key] - 1) {
            temp = ps[count[key] - 1];
            ps[count[key] - 1] = ps[i];
            ps[i] = temp;
            count[key] -= 1;
        }
    }
}

void iterate(double dt) {
    sort_into_bins(particles);
    for (int i = 0; i < PCOUNT; i++) {
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;

        // Resolve collisions.
        {
            // NOTE: Velocity of bodies will always be 0 since they are fixed
            // in place.
            double v_rel_x = particles[i].vx * dt /* - (v_body_x * dt) */;
            double v_rel_y = particles[i].vy * dt /* - (v_body_y * dt) */;

            // Window bounds (X).
            if (particles[i].x < 0 || particles[i].x > WINDOW_WIDTH) {
                // v_normal = dot(v_rel, nor) * nor
                double v_normal_x = -v_rel_x;
                double v_normal_y = 0;
                // v_tangent = v_rel - v_normal
                double v_tangent_x = 0;
                double v_tangent_y = v_rel_y;

                particles[i].x += v_normal_x - collision_friction * v_tangent_x;
                particles[i].y += v_normal_y - collision_friction * v_tangent_y;
            }

            // Window bounds (Y).
            if (particles[i].y < 0 || particles[i].y > WINDOW_HEIGHT) {
                // v_normal = dot(v_rel, nor) * nor
                double v_normal_x = 0;
                double v_normal_y = -v_rel_y;
                // v_tangent = v_rel - v_normal
                double v_tangent_x = v_rel_x;
                double v_tangent_y = 0;

                particles[i].x += v_normal_x - collision_friction * v_tangent_x;
                particles[i].y += v_normal_y - collision_friction * v_tangent_y;
            }

            // Extract any particles still outside window bounds (e.g. those
            // near corners.)
            particles[i].x = SDL_clamp(particles[i].x, 0, WINDOW_WIDTH);
            particles[i].y = SDL_clamp(particles[i].y, 0, WINDOW_HEIGHT);
        }
    }
}



//***************
//* SDL Functions
//***************

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
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

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    uint64_t new_time = SDL_GetTicksNS();
    uint64_t dt = new_time - old_time;
    old_time = new_time;
    SDL_Log("\x1b[H\x1b[0KFPS: %.2f", 1.0e9 / dt);
    SDL_Log("\x1b[0KLast Frame Time: %.2fms", dt / 1.0e6);

    iterate(dt / 1.0e9);
    draw();

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
