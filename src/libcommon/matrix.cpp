#include "matrix.h"
#include <cmath>
#include <numbers>

namespace libcommon::matrix {
  constexpr float DEGREE_TO_RADIANS = std::numbers::pi_v<float> / 180;

  Mat4 perspective(float fovy, float aspect_ratio, float near, float far) {
    // TODO:
    // See: https://www.songho.ca/opengl/gl_projectionmatrix.html

    // NOTE: The particle data is provided in 
    Mat4 matrix = {};
    matrix.m00 = 1 / aspect_ratio;
    matrix.m11 = 1;
    matrix.m22 = 1;
    matrix.m32 = 1;

    return matrix;
  }
}
