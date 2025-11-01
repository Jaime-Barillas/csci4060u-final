#pragma once

#include <catch2/catch_tostring.hpp>
#include <format>
#include <particles.h>
#include <string>

// particles::Vec3

template<>
struct Catch::StringMaker<particles::Vec3> {
  static std::string convert(const particles::Vec3 &value) {
    return std::format("({}, {}, {})", value.x, value.y, value.z);
  }
};

namespace particles {
  bool operator==(const particles::Vec3 &lhs, const particles::Vec3 &rhs) noexcept {
    return (lhs.x == rhs.x) &&
           (lhs.y == rhs.y) &&
           (lhs.z == rhs.z);
  }
}
