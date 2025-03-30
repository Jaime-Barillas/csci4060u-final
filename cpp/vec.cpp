#include <SDL3/SDL_stdinc.h>

#include "vec.hpp"

Vec Vec::operator-(const Vec &rhs) { return {x - rhs.x, y - rhs.y}; }
Vec Vec::operator*(float scalar) const { return {x * scalar, y * scalar}; }
Vec Vec::operator/(float scalar) const { return {x / scalar, y / scalar}; }

void Vec::operator=(const Vec &rhs) {
  x = rhs.x;
  y = rhs.y;
}
void Vec::operator+=(const Vec &rhs) {
  x += rhs.x;
  y += rhs.y;
}
void Vec::operator-=(const Vec &rhs) {
  x -= rhs.x;
  y -= rhs.y;
}
void Vec::operator/=(float scalar) {
  x /= scalar;
  y /= scalar;
}

float Vec::length_squared() const { return (x * x) + (y * y); }
float Vec::length() const { return SDL_sqrtf(length_squared()); }
Vec Vec::normalized() const {
  float len = length();
  return {x / len, y / len};
}

