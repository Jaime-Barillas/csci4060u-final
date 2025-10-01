#pragma once

#include <type_traits>

namespace libcommon::matrix {
  // NOTE: GLSL uses column major order by default.
  struct alignas(16) Mat4 {
    float m00;
    float m10;
    float m20;
    float m30;

    float m01;
    float m11;
    float m21;
    float m31;

    float m02;
    float m12;
    float m22;
    float m32;

    float m03;
    float m13;
    float m23;
    float m33;
  };

  static_assert(std::is_trivially_copyable_v<Mat4>, "libcommon::matrix::Mat4 must be trivially copyable");
  static_assert(std::is_standard_layout_v<Mat4>, "libcommon::matrix::Mat4 must have standard layout");

  /**
   * Create a translation along the z-axis.
   */
  Mat4 translate_z(float amount);

  /**
   * Create a rotation about the x-axis.
   */
  Mat4 rotation_x(float degrees);

  /**
   * Create a rotation about the y-axis.
   */
  Mat4 rotation_y(float degrees);

  /**
   * Create a perspective transform matrix.
   *
   * @param aspect_ratio Width / Height.
   */
  Mat4 perspective(float fovy, float aspect_ratio, float near, float far);
}

libcommon::matrix::Mat4 operator*(const libcommon::matrix::Mat4 &l, const libcommon::matrix::Mat4 &r);
