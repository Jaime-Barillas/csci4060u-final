#include "../generators.h"
#include "../misc_declarations.h" // Includes functions required by Catch2 to work on custom types.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <libcommon/vec.h>


TEST_CASE("Vec operator+", "[vec]") {
  Vec3 v1{0, 0, 0};
  Vec3 v2{1, 1, 1};
  Vec3 expected{1, 1, 1};

  REQUIRE((v1 + v2) == expected);
}

TEST_CASE("Vec operator-", "[vec]") {
  Vec3 v1{1, 1, 1};
  Vec3 v2{1, 1, 1};
  Vec3 expected{0, 0, 0};

  REQUIRE((v1 - v2) == expected);
}

TEST_CASE("Vec operator+=", "[vec]") {
  Vec3 v1{0, 0, 0};
  Vec3 v2{1, 1, 1};
  Vec3 expected{1, 1, 1};

  v1 += v2;

  REQUIRE(v1 == expected);
}

TEST_CASE("Vec operator*=", "[vec]") {
  Vec3 v1{1, 1, 1};
  float scalar = 2;
  Vec3 expected{2, 2, 2};

  v1 *= scalar;

  REQUIRE(v1 == expected);
}

TEST_CASE("Vec negate()", "[vec]") {
  Vec3 v1{1, 1, 1};
  Vec3 expected{-1, -1, -1};

  v1.negate();

  REQUIRE(v1 == expected);
}

TEST_CASE("Vec normalized()", "[vec]") {
  Vec3 v1{1, 1, 1};

  Vec3 n = v1.normalized();
  float length = std::sqrtf((n.x() * n.x()) + (n.y() * n.y()) + (n.z() * n.z()));

  REQUIRE_THAT(length, Catch::Matchers::WithinRel(1.0f, 0.01f));
}
