#include "particles.h"
#include <filesystem>
#include <print>
#include <ui.h>
#include <vector>

int main(int argc, const char **argv) {
  std::filesystem::path exe_path(argv[0]);

  std::vector<Particle> ps;

  particles::reset(ps, particles::DEFAULT_PARTICLE_COUNT, -100, 100);
  std::println("({}, {}, {})", ps[0].pos.x, ps[0].pos.y, ps[0].pos.z);
  std::println("({}, {}, {})", ps[1].pos.x, ps[1].pos.y, ps[1].pos.z);
  std::println("  Poly: {}", particles::kernel<particles::PolyKernel>(ps[0].pos, ps[1].pos));

  if (!create_ui(exe_path.parent_path().c_str())) {
    std::println("Failed to create ui");
    return 1;
  }
  while (update_ui()) {
    render_ui2();
  };
  destroy_ui();

  return 0;
}
