#ifndef PARTICLE_H
#define PARTICLE_H

#include "vec.hpp"

struct Particle {
  Vec prev_pos;
  Vec pos;
  Vec vel;
  float density;
  float near_density;
  float pressure;
  float near_pressure;
};

#endif
