#include "generators.h"
#include "misc_declarations.h" // Includes functions required by Catch2 to work on custom types.
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <cmath>
#include <format>
#include <particles.h>
#include <string>

TEST_CASE("Cell Index", "[sort]") {
  uint32_t grid_width = std::floorf((particles::RIGHT_BOUND - particles::LEFT_BOUND) / particles::SUPPORT);

  SECTION("(-1, -1, -1) maps to bin 0") {
    particles::Vec3 pos{
      .x = particles::LEFT_BOUND,
      .y = particles::LOWER_BOUND,
      .z = particles::BACKWARD_BOUND
    };

    uint32_t bin_index = particles::cell_index(pos, grid_width);
    INFO("pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")");
    REQUIRE(bin_index == 0);
  }

  SECTION("(1, 1, 1) maps to bin max") {
    uint32_t bin_count = grid_width * grid_width * grid_width;
    particles::Vec3 pos{
      .x = particles::RIGHT_BOUND,
      .y = particles::UPPER_BOUND,
      .z = particles::FORWARD_BOUND
    };

    uint32_t bin_index = particles::cell_index(pos, grid_width);
    INFO("pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")");
    REQUIRE(bin_index == (bin_count - 1));
  }

  SECTION("Within bin bounds") {
    float cell_width = (particles::RIGHT_BOUND - particles::LEFT_BOUND) / grid_width;
    auto pos = GENERATE(Catch::Generators::take(50, random_Vec3(-1.0f, 1.0f)));

    // The idea is to test that the original position is within the x/y/z
    // bounds implied by the resulting cell index.
    uint32_t bin_index = particles::cell_index(pos, grid_width);
    uint32_t x_base_index = bin_index % grid_width;
    uint32_t y_base_index = (bin_index / grid_width) % grid_width;
    uint32_t z_base_index = bin_index / (grid_width * grid_width);
    float x_start = particles::LEFT_BOUND + (x_base_index * cell_width);
    float x_end = x_start + cell_width;
    float y_start = particles::LOWER_BOUND + (y_base_index * cell_width);
    float y_end = y_start + cell_width;
    float z_start = particles::BACKWARD_BOUND + (z_base_index * cell_width);
    float z_end = z_start + cell_width;

    INFO("Index: " << bin_index << " Grid Width: " << grid_width << " Cell Width: " << cell_width);
    INFO("X: " << x_start << " <= " << pos.x << " < " << x_end);
    INFO("Y: " << y_start << " <= " << pos.y << " < " << y_end);
    INFO("Z: " << z_start << " <= " << pos.z << " < " << z_end);

    REQUIRE(((x_start <= pos.x) && (pos.x < x_end) &&
             (y_start <= pos.y) && (pos.y < y_end) &&
             (z_start <= pos.z) && (pos.z < z_end)));
  }
}

TEST_CASE("Count Sort", "[sort]") {
  uint32_t grid_width = std::floorf((particles::RIGHT_BOUND - particles::LEFT_BOUND) / particles::SUPPORT);
  auto vec_gen = random_Vec3(-1.0f, 1.0f);
  particles::Particles ps;
  ps.resize(5);

  for (auto &pos : ps.pos) {
    pos = vec_gen.get();
    vec_gen.next();
  }

  std::vector<particles::Vec3> orig_pos = ps.pos;
  particles::count_sort(ps);

  REQUIRE_THAT(orig_pos, Catch::Matchers::UnorderedEquals(ps.pos));

  std::string info;
  for (const auto &pos : ps.pos) {
    info += std::format("{}: ({}, {}, {})  ", particles::cell_index(pos, grid_width), pos.x, pos.y, pos.z);
  }
  INFO(info);

  for (int i = 1; i < ps.size(); i++) {
    uint32_t prev_cell_index = particles::cell_index(ps.pos[i - 1], grid_width);
    uint32_t curr_cell_index = particles::cell_index(ps.pos[i], grid_width);

    REQUIRE(prev_cell_index <= curr_cell_index);
  }
}

TEST_CASE("Fetch Neighbours", "[sort]") {
  uint32_t grid_width = std::floorf((particles::RIGHT_BOUND - particles::LEFT_BOUND) / particles::SUPPORT);
  particles::Particles ps;
  ps.resize(9);

  SECTION("Same Bucket") {
    for (int i = 0; i < ps.size(); i++) {
      ps.pos[i] = particles::Vec3(0 + (0.01 * i), 0, 0);
    }
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

    ps.pos[8] = particles::Vec3(-1, -1, 1); // Not a neighbour to index 0

    particles::count_sort(ps);
    particles::Particles neighbours;
    particles::fetch_neighbours(ps, 0, grid_width, neighbours);

    // Make sure to include self in neighbours to match old logic.
    ps.pos.erase(ps.pos.end());
    REQUIRE_THAT(neighbours.pos, Catch::Matchers::UnorderedEquals(ps.pos));
  }
}
