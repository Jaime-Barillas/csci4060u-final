#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include "simulator.hpp"
#include "vec.hpp"

Simulator::Simulator(
  float bound_x,
  float bound_y,
  int32_t pcount,
  float time_step,
  int32_t sim_steps,
  float gravity_y
) : bound_x{bound_x},
    bound_y{bound_y},
    pcount{pcount},
    time_step{time_step},
    sim_steps{sim_steps},
    gravity_y{gravity_y} {
  reset_particles(pcount);
}

int32_t Simulator::get_pcount() const { return pcount; }
void Simulator::set_time_step(float value) { time_step = value; }
void Simulator::set_sim_steps(int32_t value) { sim_steps = value; }
void Simulator::set_gravity_y(float value) { gravity_y = value; }

void Simulator::reset_particles(int32_t particle_count) {
  // Place particles centered in the window in a rectangular formation
  // conforming to the aspect ratio of the window.
  // Assume 16x10 aspect ratio.
  float next_multiple = SDL_ceilf(SDL_sqrtf(pcount / (16.0f * 10.0f)));
  int32_t cols = (int32_t)(next_multiple * 16.0f);
  int32_t rows = (int32_t)(next_multiple * 10.0f);
  float pos_step = PARTICLE_SIZE * 1.25f;

  pcount = particle_count;
  ps.resize(pcount);
  for (size_t i = 0; i < ps.size(); i++) {
    float x = (bound_x / 2.0f) + (i % cols) * pos_step - (cols / 2) * pos_step;
    float y = (bound_y / 2.0f) + (i / cols) * pos_step - (rows / 2) * pos_step;
    ps[i].pos = {x, y};
    ps[i].vel = {0.0f, 0.0f};
  }
}

void Simulator::integrate(float dt) {
  // 1. Apply gravity.
  for (auto &p : ps) {
    p.vel.y += gravity_y * dt;
  }

  // 2. Apply viscosity.

  // 3. Advance position.
  for (auto &p : ps) {
    p.prev_pos = p.pos;
    p.pos += p.vel * dt;
  }
  // 4. Adjust viscosity springs.

  // 5. Apply spring displacements.

  // 6. Double Density Relaxation.
  float distance;
  float contribution;
  Vec distance_vec;
  Vec accumulated_displacement;
  Vec displacement_vec;
  for (size_t i = 0; i < ps.size(); i++) {
    ps[i].density = 0.0f;
    ps[i].near_density = 0.0f;
    accumulated_displacement = {0, 0};

    for (size_t j = 0; j < ps.size(); j++) {
      if (i == j) { continue; }
      distance_vec = ps[i].pos - ps[j].pos;
      distance = distance_vec.length() / SUPPORT_RADIUS;
      if (distance < 1.0f) {
        contribution = 1.0f - distance;
        ps[i].density += contribution * contribution;
        ps[i].near_density += contribution * contribution * contribution;
      }
    }

    ps[i].pressure = 0.4 * (ps[i].density - REST_DENSITY);
    ps[i].near_pressure = 1.0 * ps[i].near_density;

    for (size_t j = 0; j < ps.size(); j++) {
      if (i == j) { continue; }
      distance_vec = ps[j].pos - ps[i].pos;
      distance = distance_vec.length() / SUPPORT_RADIUS;
      if (distance < 1.0f) {
        contribution = 1.0f - distance;
        displacement_vec = distance_vec.normalized() * (dt * dt) * ((ps[i].pressure * contribution) + (ps[i].near_pressure * contribution * contribution));
        displacement_vec /= 2.0f;
        ps[j].pos += displacement_vec;
        accumulated_displacement -= displacement_vec;
      }
    }

    ps[i].pos += accumulated_displacement;
  }

  for (auto &p : ps) {
    // 7. Collisions.

    // 8. Compute next velocity
    p.vel = (p.pos - p.prev_pos) / dt;

    // 9. Boundary checks.
    if (p.pos.x < 0.0f || p.pos.x > bound_x) {
      p.pos.x = SDL_clamp(p.pos.x, 0.0f, bound_x);
      p.vel.x *= BOUND_DAMPENING;
    }
    if (p.pos.y < 0.0f || p.pos.y > bound_y) {
      p.pos.y = SDL_clamp(p.pos.y, 0.0f, bound_y);
      p.vel.y *= BOUND_DAMPENING;
    }
  }
}

float Simulator::simulate() {
  uint64_t frame_start;
  uint64_t frame_total = 0;

  for (int32_t step = 0; step < sim_steps; step++) {
    frame_start = SDL_GetPerformanceCounter();
    integrate(1.0f / time_step);
    frame_total += SDL_GetPerformanceCounter() - frame_start;
  }

  return (float)frame_total / sim_steps / SDL_GetPerformanceFrequency();
}

void Simulator::draw(SDL_Renderer *r) {
  SDL_FRect rect;
  float half_psize = PARTICLE_SIZE / 2.0f;

  SDL_SetRenderDrawColor(r, 228, 228, 228, 255);
  for (auto &p : ps) {
    rect = {p.pos.x - half_psize, p.pos.y - half_psize, PARTICLE_SIZE, PARTICLE_SIZE};
    SDL_RenderFillRect(r, &rect);
  }
}
