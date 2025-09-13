#pragma once

#include <concepts>
#include <cstdint>

namespace util {
  /**
   * Raise `base` to the power of `power`.
   */
  template <typename T>
  requires (std::integral<T> || std::floating_point<T>)
  consteval T pow(T base, uint32_t power) {
    T result = base;
    for (uint32_t i = 1; i < power; i++) {
      result *= base;
    }
    return result;
  }
}
