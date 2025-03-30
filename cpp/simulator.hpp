#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdint.h>
#include <vector>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>

#include "particle.hpp"

constexpr float PARTICLE_SIZE = 8;
constexpr float PARTICLE_MASS = 2.5f;
constexpr float BOUND_DAMPENING = -0.5f;
constexpr float SUPPORT_RADIUS = 16.0f;
constexpr float GAS_CONSTANT = 2000.0f;
constexpr float NEAR_GAS_CONSTANT = 30000.0f;
constexpr float REST_DENSITY = 300.0f;
constexpr float VISCOSITY = 200.0f;

// Precompute coefficients of kernel functions.
constexpr float SUPPORT_RADIUS_SQR = SUPPORT_RADIUS * SUPPORT_RADIUS;
constexpr float SUPPORT_RADIUS_POW5 = SUPPORT_RADIUS_SQR * SUPPORT_RADIUS_SQR * SUPPORT_RADIUS;
constexpr float SUPPORT_RADIUS_POW8 = SUPPORT_RADIUS_POW5 * SUPPORT_RADIUS_SQR * SUPPORT_RADIUS;
constexpr float POLY6 = 4.0f / (SDL_PI_F * SUPPORT_RADIUS_POW8);
constexpr float SPIKY_GRADIENT = -10.0f / (SDL_PI_F * SUPPORT_RADIUS_POW5);
constexpr float VISC_LAPLACIAN = 40.0f / (SDL_PI_F * SUPPORT_RADIUS_POW5);

class Simulator {
  float bound_x;
  float bound_y;
  int32_t pcount;
  float time_step;
  int32_t sim_steps;
  float gravity_y;

  std::vector<Particle> ps;

  void calculate_forces();
  void integrate(float dt);

public:
  Simulator(
    float bound_x,
    float bound_y,
    int32_t pcount,
    float time_step,
    int32_t sim_steps,
    float gravity_y
  );

  int32_t get_pcount() const;
  void set_time_step(float value);
  void set_sim_steps(int32_t value);
  void set_gravity_y(float value);
  void reset_particles(int32_t particle_count);
  void initialize_integration();
  float simulate();
  void draw(SDL_Renderer *r);
};

#endif
