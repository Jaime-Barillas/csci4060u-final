#include "matrix.h"
#include <cmath>
#include <numbers>

namespace libcommon::matrix {
  constexpr float DEGREE_TO_RADIANS = std::numbers::pi_v<float> / 180;

  Mat4 translate_z(float amount) {
    Mat4 matrix = {};

    matrix.m00 = 1.0f;
    matrix.m11 = 1.0f;
    matrix.m22 = 1.0f;
    matrix.m33 = 1.0f;

    matrix.m23 = amount;

    return matrix;
  }

  Mat4 rotation_x(float degrees) {
    const float radians = degrees * DEGREE_TO_RADIANS;
    const float cos = std::cosf(radians);
    const float sin = std::sinf(radians);

    Mat4 matrix = {};

    matrix.m00 = 1.0f;
    matrix.m11 =  cos;
    matrix.m12 = -sin;
    matrix.m21 =  sin;
    matrix.m22 =  cos;
    matrix.m33 = 1.0f;

    return matrix;
  }

  Mat4 rotation_y(float degrees) {
    const float radians = degrees * DEGREE_TO_RADIANS;
    const float cos = std::cosf(radians);
    const float sin = std::sinf(radians);

    Mat4 matrix = {};

    matrix.m00 =  cos;
    matrix.m02 =  sin;
    matrix.m11 =  1.0f;
    matrix.m20 = -sin;
    matrix.m22 =  cos;
    matrix.m33 = 1.0f;

    return matrix;
  }

  Mat4 perspective(float fovy, float aspect_ratio, float near, float far) {
    // TODO:
    // See: https://www.songho.ca/opengl/gl_projectionmatrix.html

    // NOTE: The particle data is provided in 
    Mat4 matrix = {};
    matrix.m00 = 1.0f / aspect_ratio;
    matrix.m11 = 1.0f;
    matrix.m22 = 1.0f;
    matrix.m32 = 1.0f;

    return matrix;
  }
}

libcommon::matrix::Mat4 operator*(const libcommon::matrix::Mat4 &l, const libcommon::matrix::Mat4 &r) {
  libcommon::matrix::Mat4 matrix = {};

  matrix.m00 = (l.m00 * r.m00) + (l.m01 * r.m10) + (l.m02 * r.m20) + (l.m03 * r.m30);
  matrix.m01 = (l.m00 * r.m01) + (l.m01 * r.m11) + (l.m02 * r.m21) + (l.m03 * r.m31);
  matrix.m02 = (l.m00 * r.m02) + (l.m01 * r.m12) + (l.m02 * r.m22) + (l.m03 * r.m32);
  matrix.m03 = (l.m00 * r.m03) + (l.m01 * r.m13) + (l.m02 * r.m23) + (l.m03 * r.m33);

  matrix.m10 = (l.m10 * r.m00) + (l.m11 * r.m10) + (l.m12 * r.m20) + (l.m13 * r.m30);
  matrix.m11 = (l.m10 * r.m01) + (l.m11 * r.m11) + (l.m12 * r.m21) + (l.m13 * r.m31);
  matrix.m12 = (l.m10 * r.m02) + (l.m11 * r.m12) + (l.m12 * r.m22) + (l.m13 * r.m32);
  matrix.m13 = (l.m10 * r.m03) + (l.m11 * r.m13) + (l.m12 * r.m23) + (l.m13 * r.m33);

  matrix.m20 = (l.m20 * r.m00) + (l.m21 * r.m10) + (l.m22 * r.m20) + (l.m23 * r.m30);
  matrix.m21 = (l.m20 * r.m01) + (l.m21 * r.m11) + (l.m22 * r.m21) + (l.m23 * r.m31);
  matrix.m22 = (l.m20 * r.m02) + (l.m21 * r.m12) + (l.m22 * r.m22) + (l.m23 * r.m32);
  matrix.m23 = (l.m20 * r.m03) + (l.m21 * r.m13) + (l.m22 * r.m23) + (l.m23 * r.m33);

  matrix.m30 = (l.m30 * r.m00) + (l.m31 * r.m10) + (l.m32 * r.m20) + (l.m33 * r.m30);
  matrix.m31 = (l.m30 * r.m01) + (l.m31 * r.m11) + (l.m32 * r.m21) + (l.m33 * r.m31);
  matrix.m32 = (l.m30 * r.m02) + (l.m31 * r.m12) + (l.m32 * r.m22) + (l.m33 * r.m32);
  matrix.m33 = (l.m30 * r.m03) + (l.m31 * r.m13) + (l.m32 * r.m23) + (l.m33 * r.m33);

  return matrix;
}
