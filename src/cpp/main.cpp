#include "particles.h"
#include <print>
#include <ui.h>
#include <vector>

int main() {
  std::vector<Particle> ps;

  particles::reset(ps, particles::DEFAULT_PARTICLE_COUNT, -100, 100);
  std::println("({}, {}, {})", ps[0].pos.x, ps[0].pos.y, ps[0].pos.z);
  std::println("({}, {}, {})", ps[1].pos.x, ps[1].pos.y, ps[1].pos.z);
  std::println("  Poly: {}", particles::kernel<particles::PolyKernel>(ps[0].pos, ps[1].pos));

  return 0;
}
