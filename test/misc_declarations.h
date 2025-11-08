#pragma once

#include <catch2/catch_tostring.hpp>
#include <format>
#include <libcommon/vec.h>

template<>
struct Catch::StringMaker<Vec3> {
  static std::string convert(const Vec3 &value) {
    return std::format("({}, {}, {})", value.x(), value.y(), value.z());
  }
};

bool operator==(const Vec3 &lhs, const Vec3 &rhs) noexcept;
