#include "../generators.h"
#include "../misc_declarations.h" // Includes functions required by Catch2 to work on custom types.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <cmath>
#include <cpp/neighbours.h>
#include <cpp/particles.h>
#include <cpp/sim_opts.h>

TEST_CASE("Cell Index", "[sort]") {
  uint32_t grid_width = std::floorf((X_BOUNDS.y() - X_BOUNDS.x()) / SUPPORT);
  Neighbours ns;

  SECTION("(-1, -1, -1) maps to bin 0") {
    Vec3 pos{
      X_BOUNDS.x(),
      Y_BOUNDS.x(),
      Z_BOUNDS.x()
    };

    uint32_t bin_index = ns.cell_index(pos, grid_width);
    INFO("pos: (" << pos.x() << ", " << pos.y() << ", " << pos.z() << ")");
    REQUIRE(bin_index == 0);
  }

  SECTION("(1, 1, 1) maps to bin max") {
    uint32_t bin_count = grid_width * grid_width * grid_width;
    Vec3 pos{
      X_BOUNDS.y(),
      Y_BOUNDS.y(),
      Z_BOUNDS.y()
    };

    uint32_t bin_index = ns.cell_index(pos, grid_width);
    INFO("pos: (" << pos.x() << ", " << pos.y() << ", " << pos.z() << ")");
    REQUIRE(bin_index == (bin_count - 1));
  }

  SECTION("Within bin bounds") {
    float cell_width = (X_BOUNDS.y() - X_BOUNDS.x()) / grid_width;
    auto pos = GENERATE(Catch::Generators::take(50, random_Vec3(-1.0f, 1.0f)));

    // The idea is to test that the original position is within the x/y/z
    // bounds implied by the resulting cell index.
    uint32_t bin_index = ns.cell_index(pos, grid_width);
    uint32_t x_base_index = bin_index % grid_width;
    uint32_t y_base_index = (bin_index / grid_width) % grid_width;
    uint32_t z_base_index = bin_index / (grid_width * grid_width);
    float x_start = X_BOUNDS.x() + (x_base_index * cell_width);
    float x_end = x_start + cell_width;
    float y_start = Y_BOUNDS.x() + (y_base_index * cell_width);
    float y_end = y_start + cell_width;
    float z_start = Z_BOUNDS.x() + (z_base_index * cell_width);
    float z_end = z_start + cell_width;

    INFO("Index: " << bin_index << " Grid Width: " << grid_width << " Cell Width: " << cell_width);
    INFO("X: " << x_start << " <= " << pos.x() << " < " << x_end);
    INFO("Y: " << y_start << " <= " << pos.y() << " < " << y_end);
    INFO("Z: " << z_start << " <= " << pos.z() << " < " << z_end);

    REQUIRE(((x_start <= pos.x()) && (pos.x() < x_end) &&
             (y_start <= pos.y()) && (pos.y() < y_end) &&
             (z_start <= pos.z()) && (pos.z() < z_end)));
  }
}

TEST_CASE("Count Sort", "[sort]") {
  uint32_t grid_width = std::floorf((RIGHT_BOUND - LEFT_BOUND) / SUPPORT);
  Neighbours ns;
  SimOpts sim_opts{
    .bench_mode = false,
    .particle_count = 5,
    .particle_radius = 0,
    .gas_constant = 0,
    .rest_density = 0,
    .support = SUPPORT,
    .viscosity_constant = 0,
  };
  auto vec_gen = random_Vec3(-1.0f, 1.0f);
  Particles ps;
  ps.resize(sim_opts.particle_count);

  for (auto &pos : ps.pos) {
    pos = vec_gen.get();
    vec_gen.next();
  }

  std::vector<Vec3> orig_pos = ps.pos;
  ns.process(ps, sim_opts);

  REQUIRE_THAT(orig_pos, Catch::Matchers::UnorderedEquals(ps.pos));

  std::string info;
  for (const auto &pos : ps.pos) {
    info += std::format("{}: ({}, {}, {})  ", ns.cell_index(pos, grid_width), pos.x(), pos.y(), pos.z());
  }
  INFO(info);

  for (int i = 1; i < ps.size(); i++) {
    uint32_t prev_cell_index = ns.cell_index(ps.pos[i - 1], grid_width);
    uint32_t curr_cell_index = ns.cell_index(ps.pos[i], grid_width);

    REQUIRE(prev_cell_index <= curr_cell_index);
  }
}

TEST_CASE("Fetch Neighbours", "[sort]") {
  uint32_t grid_width = std::floorf((X_BOUNDS.y() - X_BOUNDS.x()) / SUPPORT);
  Neighbours ns;
  SimOpts sim_opts{
    .bench_mode = false,
    .particle_count = 9,
    .particle_radius = 0,
    .gas_constant = 0,
    .rest_density = 0,
    .support = SUPPORT,
    .viscosity_constant = 0,
  };
  Particles ps;
  ps.resize(sim_opts.particle_count);

  SECTION("Same Bucket") {
    for (int i = 0; i < ps.size(); i++) {
      ps.pos[i] = Vec3{0.0f + (0.01f * i), 0, 0};
    }

    ns.process(ps, sim_opts);
    Particles neighbours;
    ns.neighbours_near(ps, ps.pos[0], sim_opts, neighbours);

    // Make sure to include self in neighbours to match old logic.
    REQUIRE_THAT(neighbours.pos, Catch::Matchers::UnorderedEquals(ps.pos));
  }

  SECTION("Low extreme") {
    ps.pos[0] = Vec3{-1, -1, -1};
    ps.pos[1] = Vec3{-0.6, -1, -1};
    ps.pos[2] = Vec3{-1, -0.6, -1};
    ps.pos[3] = Vec3{-0.6, -0.6, -1};

    ps.pos[4] = Vec3{-1, -1, -0.6};
    ps.pos[5] = Vec3{-0.6, -1, -0.6};
    ps.pos[6] = Vec3{-1, -0.6, -0.6};
    ps.pos[7] = Vec3{-0.6, -0.6, -0.6};

    ps.pos[8] = Vec3{-1, -1, 1}; // Not a neighbour to index 0

    ns.process(ps, sim_opts);
    Particles neighbours;
    ns.neighbours_near(ps, ps.pos[0], sim_opts, neighbours);

    // Make sure to include self in neighbours to match old logic.
    ps.pos.erase(ps.pos.end());
    REQUIRE_THAT(neighbours.pos, Catch::Matchers::UnorderedEquals(ps.pos));
  }
}
