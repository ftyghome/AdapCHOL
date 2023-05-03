#pragma once

#include <functional>
#include <chrono>


#define TIMED_RUN
#ifdef TIMED_RUN

inline int64_t timedRun(const std::function<void(void)> &func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count();
}

#else

int64_t timedRun(const std::function<void(void)> &func) {
    func();
    return 0;
}

#endif

inline int64_t timedRunAlways(const std::function<void(void)> &func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count();
}