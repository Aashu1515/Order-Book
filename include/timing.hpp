#pragma once
#include <cstdint>
#include <time.h>

inline std::uint64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000ull + ts.tv_nsec;
}
