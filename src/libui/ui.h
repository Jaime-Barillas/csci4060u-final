#pragma once

struct Vec3 {
  float x;
  float y;
  float z;
};

struct Particle {
  struct Vec3 pos;
  struct Vec3 vel;
  float density;
  float pressure;
};
