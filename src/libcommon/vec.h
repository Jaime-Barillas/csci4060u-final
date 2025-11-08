#pragma once

#include <cmath>
#include <cstddef>
#include <type_traits>


template <size_t N> requires(N >= 2)
struct Vec {
  float data[N];

  /****************/
  /* Constructors */
  /****************/
  constexpr Vec() { }

  constexpr Vec(float x, float y) requires(N == 2) {
    data[0] = x;
    data[1] = y;
  }

  constexpr Vec(float x, float y, float z) requires(N == 3) {
    data[0] = x;
    data[1] = y;
    data[2] = z;
  }

  constexpr Vec(float x, float y, float z, float w) requires(N == 4) {
    data[0] = x;
    data[1] = y;
    data[2] = z;
    data[3] = w;
  }

  /**************/
  /* Properties */
  /**************/
  float x() const { return data[0]; }
  float y() const { return data[1]; }
  float z() const requires(N >= 3) { return data[2]; }

  void x(float value) { data[0] = value; }
  void y(float value) { data[1] = value; }
  void z(float value) requires(N >= 3) { data[2] = value; }

  /*************/
  /* Functions */
  /*************/
  Vec operator+(const Vec &other) const {
    Vec v;

    for (size_t i = 0; i < N; i++) {
      v.data[i] = data[i] + other.data[i];
    }

    return v;
  }

  Vec operator-(Vec other) const {
    Vec v;

    for (size_t i = 0; i < N; i++) {
      v.data[i] = data[i] - other.data[i];
    }

    return v;
  }

  Vec operator*(float scalar) {
    Vec v;

    for (size_t i = 0; i < N; i++) {
      v.data[i] = data[i] * scalar;
    }

    return v;
  }

  void operator+=(Vec other) {
    for (size_t i = 0; i < N; i++) {
      data[i] += other.data[i];
    }
  }

  void operator*=(float scalar) {
    for (size_t i = 0; i < N; i++) {
      data[i] *= scalar;
    }
  }

  void negate() {
    for (size_t i = 0; i < N; i++) {
      data[i] = -data[i];
    }
  }

  float length_squared() const {
    float length = 0.0f;

    for (size_t i = 0; i < N; i++) {
      length += data[i] * data[i];
    }

    return length;
  }

  float length() const {
    float length = length_squared();
    return std::sqrtf(length);
  }

  Vec normalized() const {
    Vec v;

    float length = 0.0f;
    for (size_t i = 0; i < N; i++) {
      v.data[i] = data[i];
      length += data[i] * data[i];
    }

    length = std::sqrtf(length);
    for (size_t i = 0; i < N; i++) {
      v.data[i] /= length;
    }

    return v;
  }

  // Convenience function for use when transferring vector data to the GPU
  // (where std140 layout must be followed.)
  void copy_vec3(Vec<3> other, float w = 1.0f) requires(N == 4) {
    data[0] = other.data[0];
    data[1] = other.data[1];
    data[2] = other.data[2];
    data[3] = w;
  }
};

typedef Vec<2> Vec2;
typedef Vec<3> Vec3;
typedef Vec<4> Vec4;


// Ensure fancy C++ vectors are identical to simple C structs so they can be
// passed through to SDL.

extern "C" {
  typedef struct { float x, y; } _Vec2C;
  typedef struct { float x, y, z; } _Vec3C;
  typedef struct { float x, y, z, w; } _Vec4C;
}

static_assert( sizeof(Vec2) == sizeof(_Vec2C), "Vec<2> size does not match expected C size" );
static_assert( sizeof(Vec3) == sizeof(_Vec3C), "Vec<3> size does not match expected C size" );
static_assert( sizeof(Vec4) == sizeof(_Vec4C), "Vec<4> size does not match expected C size" );
static_assert( alignof(Vec2) == alignof(_Vec2C), "Vec<2> alignment does not match expected C alignment" );
static_assert( alignof(Vec3) == alignof(_Vec3C), "Vec<3> alignment does not match expected C alignment" );
static_assert( alignof(Vec4) == alignof(_Vec4C), "Vec<4> alignment does not match expected C alignment" );
static_assert( std::is_standard_layout_v<Vec2>, "Vec<2> does not follow standard layout" );
static_assert( std::is_standard_layout_v<Vec3>, "Vec<3> does not follow standard layout" );
static_assert( std::is_standard_layout_v<Vec4>, "Vec<4> does not follow standard layout" );
