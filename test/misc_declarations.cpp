#include "misc_declarations.h"

bool operator==(const Vec3 &lhs, const Vec3 &rhs) noexcept {
  return (lhs.x() == rhs.x()) &&
         (lhs.y() == rhs.y()) &&
         (lhs.z() == rhs.z());
}
