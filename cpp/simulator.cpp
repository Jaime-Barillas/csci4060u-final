#ifdef ENABLE_PARALLELISM
#include <omp.h>
#endif
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
    ps[i].force_next = {0.0f, 0.0f};
    ps[i].density = 0.0f;
    ps[i].near_density = 0.0f;
    ps[i].pressure = 0.0f;
    ps[i].near_pressure = 0.0f;
  }
}

void Simulator::calculate_forces() {
  // A(pi) = Sum( m * A(pj)/pj_density * W(dist, support_radius) )
  float length;
  float contribution;
  Vec distance;

  // Density and pressure.
#ifdef ENABLE_PARALLELISM
  #pragma omp parallel for private(length, contribution, distance)
#endif
  for (int32_t i = 0; i < ps.size(); i++) {
    ps[i].density = 0.0f;
    ps[i].near_density = 0.0f;
    ps[i].pressure = 0.0f;
    ps[i].near_pressure = 0.0f;

    for (int32_t j = 0; j < ps.size(); j++) {
      // 1. Calculate density including self (Smoothing kernel).
      //    (support_radius^2 - dist^2)^3
      distance = ps[j].pos - ps[i].pos;
      length = distance.length_squared();
      if (length < SUPPORT_RADIUS_SQR) {
        contribution = SUPPORT_RADIUS_SQR - length;
        ps[i].density += PARTICLE_MASS * POLY6 * contribution * contribution * contribution;
        ps[i].near_density += PARTICLE_MASS * (5/SDL_PI_F/(SUPPORT_RADIUS_POW8 * SUPPORT_RADIUS_SQR)) * SDL_powf(contribution, 4.0f);
      }
    }

    // 2. Calculate pressure.
    ps[i].pressure = GAS_CONSTANT * (ps[i].density - REST_DENSITY);
    ps[i].near_pressure = NEAR_GAS_CONSTANT * ps[i].near_density;
  }

  // 3. Calculate forces.
  Vec pressure_force;
  Vec viscosity_force;
#ifdef ENABLE_PARALLELISM
  #pragma omp parallel for private(length, contribution, distance, pressure_force, viscosity_force)
#endif
  for (int32_t i = 0; i < ps.size(); i++) {
    ps[i].force_next = {0.0f, 0.0f};
    pressure_force = {0.0f, 0.0f};
    viscosity_force = {0.0f, 0.0f};

    for (int32_t j = 0; j < ps.size(); j++) {
      if (i == j) { continue; }

      distance = ps[j].pos - ps[i].pos;
      length = distance.length();
      if (length < SUPPORT_RADIUS) {
        contribution = SUPPORT_RADIUS - length;

        // a) Viscosity (Laplacian of smoothing kernel).
        viscosity_force += ((ps[j].vel - ps[i].vel) / ps[j].density) * VISCOSITY * PARTICLE_MASS * VISC_LAPLACIAN * contribution;

        // b) Pressure forces (Gradient of smoothing kernel).
        //    (support_radius - dist)^3
        contribution = SDL_powf(contribution, 3.0f);
        pressure_force -= distance.normalized() *0.05f* PARTICLE_MASS * (ps[i].pressure + ps[j].pressure) / (2.0f * ps[j].density) * SPIKY_GRADIENT * contribution;
        pressure_force -= distance.normalized() * PARTICLE_MASS * (ps[i].near_pressure + ps[j].near_pressure) / (2.0f * ps[j].density) * SPIKY_GRADIENT * contribution;
      }
    }

    ps[i].force_next += pressure_force;
    ps[i].force_next += viscosity_force;
    // c) External forces (Gravity).
    ps[i].force_next.y += 0.1f* gravity_y * PARTICLE_MASS / ps[i].density;
  }
}

// Initialize values for leapfrog method.
void Simulator::initialize_integration() {
  calculate_forces();

#ifdef ENABLE_PARALLELISM
  #pragma omp parallel for
#endif
  for (int32_t i = 0; i < ps.size(); i++) {
    ps[i].accel = ps[i].force_next / ps[i].density;
  }
}

void Simulator::integrate(float dt) {
#ifdef ENABLE_PARALLELISM
  #pragma omp parallel for
#endif
  for (int32_t i = 0; i < ps.size(); i++) {
    // 4. Integrate.
    // Leapfrog method (synchronized form).
    ps[i].vel += (ps[i].accel + (ps[i].force_next / ps[i].density)) * 0.5f * dt;
    ps[i].pos += (ps[i].vel * dt) + (ps[i].accel * 0.5f * dt * dt);
    ps[i].accel = ps[i].force_next / ps[i].density;

    // 5. Boundary checks.
    if (ps[i].pos.x < 0.0f || ps[i].pos.x > bound_x) {
      ps[i].pos.x = SDL_clamp(ps[i].pos.x, 0.0f, bound_x);
      ps[i].vel.x *= BOUND_DAMPENING;
    }
    if (ps[i].pos.y < 0.0f || ps[i].pos.y > bound_y) {
      ps[i].pos.y = SDL_clamp(ps[i].pos.y, 0.0f, bound_y);
      ps[i].vel.y *= BOUND_DAMPENING;
    }
  }
}

float Simulator::simulate() {
  uint64_t frame_start;
  uint64_t frame_total = 0;

  for (int32_t step = 0; step < sim_steps; step++) {
    frame_start = SDL_GetPerformanceCounter();
    calculate_forces();
    integrate(0.1f / time_step);
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
