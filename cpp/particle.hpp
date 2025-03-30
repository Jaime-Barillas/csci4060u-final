#ifndef PARTICLE_H
#define PARTICLE_H

#include "vec.hpp"

struct Particle {
  Vec pos;
  Vec vel;
  Vec vel_full_step;
  Vec force;
  float density;
  float near_density;
  float pressure;
  float near_pressure;
};

#endif
