#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdint.h>
#include <vector>

#include <SDL3/SDL_render.h>

#include "particle.hpp"

constexpr float PARTICLE_SIZE = 8;
constexpr float BOUND_DAMPENING = -0.8f;

class Simulator {
  float bound_x;
  float bound_y;
  int32_t pcount;
  float time_step;
  int32_t sim_steps;
  float gravity_y;
  float frame_times[4];

  std::vector<Particle> ps;

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

  void set_time_step(float value);
  void set_sim_steps(int32_t value);
  void set_gravity_y(float value);
  void reset_particles(int32_t particle_count);
  float simulate();
  void draw(SDL_Renderer *r);
};

#endif
