#include "SDL3/SDL_rect.h"
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_render.h"
#include <SDL3/SDL_timer.h>
#include <string.h>

#define PCOUNT 60
#define GRAVITY 9.81

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

typedef struct Particle_T {
    double vx;
    double vy;
    double x;
    double y;
} Particle;

Particle particles[PCOUNT];
Uint64 neighbours[PCOUNT];
Uint64 neighbour_count = 0;
double interaction_radius = 40;
Uint64 old_time = 0;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example HUMAN READABLE NAME", "1.0", "com.example.CATEGORY-NAME");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/CATEGORY/NAME", 640, 480, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(renderer, 1);

    memset(particles, 0, sizeof(particles));
    memset(neighbours, 0, sizeof(neighbours));
    for (int i = 0; i < PCOUNT; i++) {
        particles[i].x = SDL_randf() * 60;
        particles[i].y = SDL_randf() * 40;
        particles[i].vx = 0.0;
        particles[i].vy = 0.0;
    }
    old_time = SDL_GetTicksNS();
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
    Uint64 new_time = SDL_GetTicksNS();
    double dt = (float)(new_time - old_time) / 1000000000;
    old_time = new_time;

    for (int i = 0; i < PCOUNT; i++) {
        // 1. Apply Gravity to Velocity
        particles[i].vy += dt * GRAVITY;

        // 3. Advance to predicted Position
        double oldx = particles[i].x;
        double oldy = particles[i].y;
        particles[i].x += dt * particles[i].vx;
        particles[i].y += dt * particles[i].vy;

        // 6. Double Density Relaxation
        double density = 0;
        double density_near = 0;
        double pressure = 0;
        double pressure_near = 0;

        // Compute neighbours: O(n^2)
        neighbour_count = 0;
        for (int j = 0; j < PCOUNT; j++) {
            if (j == i) continue;
            double rx = particles[j].x - particles[i].x;
            double ry = particles[j].y - particles[i].y;
            double r = SDL_sqrt(rx*rx + ry*ry);

            // Find insertion point.
            for (Uint64 k = 0; k < PCOUNT; k++) {
                double dx = particles[neighbours[k]].x - particles[i].x;
                double dy = particles[neighbours[k]].y - particles[i].y;
                double d = SDL_sqrt(dx*dx + dy*dy);
                SDL_Log("i: %d, j: %d, r: %f, d: %f\n", i, j, r, d);

                if (r < d) {
                    for (Uint64 l = PCOUNT - 1; l > k; l--) {
                        neighbours[l] = neighbours[l - 1];
                    }
                    neighbours[k] = j;
                    neighbour_count += 1;
                    break;
                }
            }
        }

        for (Uint64 j = 0; j < neighbour_count; j++) {
            double rx = particles[neighbours[j]].x - particles[i].x;
            double ry = particles[neighbours[j]].y - particles[i].y;
            double r = SDL_sqrt(rx*rx + ry*ry);

            double dterm = 1.0 - (r / interaction_radius);
            density += dterm * dterm;
            density_near += dterm * dterm * dterm;
        }
        double k = 0.5;
        double k_near = 0.5;
        double rest_density = 0.5;
        pressure = k * (density - rest_density);
        pressure_near = k_near * density_near;
        double dx = 0;
        double dy = 0;
        SDL_Log("i: %d, Ns:%llu\n", i, neighbour_count);
        for (Uint64 j = 0; j < neighbour_count; j++) {
            double rx = particles[neighbours[j]].x - particles[i].x;
            double ry = particles[neighbours[j]].y - particles[i].y;
            double r = SDL_sqrt(rx*rx + ry*ry);
            // SDL_Log("i: %d, j:%llu, rx: %f, ry: %f, r: %f\n", i, j, rx, ry, r);

            if (r / interaction_radius < 1.0) {
                rx /= r;
                ry /= r;

                double dterm = 1.0 - (r / interaction_radius);
                double D_scale = (dt*dt) * ((pressure * dterm) + (pressure_near * dterm * dterm));
                double Dx = D_scale * rx;
                double Dy = D_scale * ry;

                particles[neighbours[j]].x += Dx / 2;
                particles[neighbours[j]].y += Dy / 2;
                dx -= Dx / 2;
                dy -= Dy / 2;
            }
        }
        particles[i].x += dx;
        particles[i].y += dy;
        SDL_Log("dx: %f, dy: %f\n", dx, dy);

        // 8. Compute next Velocity
        particles[i].vx = (particles[i].x - oldx) / dt;
        particles[i].vy = (particles[i].y - oldy) / dt;

        // 9. Wrap arround window
        if (particles[i].y > 480) {
            particles[i].y = 0;
        }
    }

    // 2. Apply Viscosity Impulses
    // ...

        // 4. Update Springs (Optional)
        // ...

    // 5. Apply Spring Displacements (Optional)
    // ...

    // 7. Resolve Collisions
    // ...

    // Render the particles
    SDL_SetRenderDrawColorFloat(renderer, 0.8, 0.5, 0.6, 1.0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColorFloat(renderer, 0.6, 0.5, 0.8, 1.0);
    SDL_FRect rect;
    rect.w = 5;
    rect.h = 5;
    for (int i = 0; i < PCOUNT; i++) {
        rect.x = particles[i].x;
        rect.y = particles[i].y;
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_RenderPresent(renderer);
    SDL_DelayNS(((1e9 * 16) / 1000) - (new_time - old_time));
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}
