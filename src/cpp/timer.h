#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class FrameTimer {
  std::vector<uint64_t> frame_times;
  uint64_t start_timestamp;
  size_t current_frame;

  public:
    FrameTimer(uint32_t buffer_size);

    void record_start();
    void record_end();
    double average_millis() const;
    size_t recorded_frames() const;
};
