#include <SDL3/SDL_stdinc.h>

#include "vec.hpp"

Vec Vec::operator-(const Vec &rhs) { return {x - rhs.x, y - rhs.y}; }
Vec Vec::operator*(float scalar) const { return {x * scalar, y * scalar}; }

void Vec::operator+=(const Vec &rhs) {
  x += rhs.x;
  y += rhs.y;
}

float Vec::length() const { return SDL_sqrtf((x * x) + (y * y)); }
