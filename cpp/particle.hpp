#ifndef PARTICLE_H
#define PARTICLE_H

#include "vec.hpp"

struct Particle {
  Vec pos;
  Vec vel;
  Vec accel;
  Vec force_next;
  float density;
  float near_density;
  float pressure;
  float near_pressure;
};

#endif
