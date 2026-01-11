#pragma once

#include <chrono>

inline uint32_t globalRngState = 123456789u;

// Xorshift Random Generator - Marsaglia
inline uint32_t xorshift32(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline float toFloat01(uint32_t v) {
    return (v >> 8) * (1.0f / 16777216.0f);
}

inline void init_random() {
    auto time_now = std::chrono::high_resolution_clock::now();
    auto dur = time_now.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(dur);
    globalRngState = static_cast<uint32_t>(nanos.count());
}

inline float genRandom01() {
    return toFloat01(xorshift32(globalRngState));
}
