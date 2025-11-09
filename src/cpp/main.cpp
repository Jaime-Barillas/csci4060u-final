#include "sim.h"
#include <filesystem>
#include <print>


constexpr uint32_t DEFAULT_PARTICLE_COUNT = 1024;

int main(int argc, const char **argv) {
  std::filesystem::path exe_path(argv[0]);
  bool bench_mode = false;
  uint32_t particle_count = DEFAULT_PARTICLE_COUNT;

  for (size_t i = 1; i < argc; i++) {
    std::string_view arg(argv[i]);
    if (arg == "--bench") {
      bench_mode = true;
    } else {
      auto res = std::from_chars(arg.begin(), arg.end(), particle_count);

      // If the entire arg was not consumed, default to a known good value.
      // FIXME: Allow non-multiples of 64.
      if (res.ptr != arg.end() || particle_count % 64 != 0) {
        particle_count = DEFAULT_PARTICLE_COUNT;
      }
    }
  }

  Sim simulator(exe_path, particle_count, bench_mode);
  try {
    simulator.init();
    simulator.run_loop();
  } catch(std::runtime_error err) {
    std::println("Error");
  }

  return 0;
}
