#include "generators.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <cmath>
#include <particles.h>

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
