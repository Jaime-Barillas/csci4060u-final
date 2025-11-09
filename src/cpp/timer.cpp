#include "timer.h"
#include <algorithm>
#include <cstdint>
#include <SDL3/SDL_timer.h>

FrameTimer::FrameTimer(uint32_t buffer_size) : current_frame{0} {
  frame_times.resize(buffer_size);
}

void FrameTimer::record_start() {
  start_timestamp = SDL_GetPerformanceCounter();
}

void FrameTimer::record_end() {
  uint64_t end_timestamp = SDL_GetPerformanceCounter();
  uint64_t frame_time = end_timestamp - start_timestamp;
  frame_times[current_frame % frame_times.size()] = frame_time;
  current_frame += 1;
}

double FrameTimer::average_millis() const {
  double total_seconds = 0.0f;
  size_t frame_count = std::min(current_frame + 1, frame_times.size());

  for (auto time_span : frame_times) {
    total_seconds += time_span;
  }

  total_seconds /= (SDL_GetPerformanceFrequency());
  return (total_seconds * 1'000) / frame_count;
}

size_t FrameTimer::recorded_frames() const {
  return std::min(current_frame, frame_times.size());
}
