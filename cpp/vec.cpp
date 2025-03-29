#include "vec.hpp"

Vec Vec::operator*(float scalar) const { return {x * scalar, y * scalar}; }

void Vec::operator+=(const Vec &rhs) {
  x += rhs.x;
  y += rhs.y;
}
