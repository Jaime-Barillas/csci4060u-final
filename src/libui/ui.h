#pragma once

#include <vector>

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

extern "C" {
  char create_ui(const char *exe_path);
  char update_ui(void);
  void render_ui(const struct Particle *const *particles);
  void render_ui2();
  void test_compute(std::vector<Particle> ps);
  void destroy_ui(void);
}
