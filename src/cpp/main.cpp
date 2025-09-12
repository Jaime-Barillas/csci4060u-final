#include "particles.h"
#include <ui.h>
#include <vector>

int main() {
  std::vector<Particle> ps;

  Particles::reset(ps, Particles::DEFAULT_PARTICLE_COUNT, -100, 100);

  return 0;
}
