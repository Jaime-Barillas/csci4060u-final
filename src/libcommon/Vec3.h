#pragma once

extern "C" {
  struct Vec3 {
    float x;
    float y;
    float z;
    float _pad; // vec3 are padded to 16-byte alignment in Vulkan.
  };

  struct Vec4 {
    float x;
    float y;
    float z;
    float w;
  };
}

