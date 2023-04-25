#include "utils.h"
#include <functional>
#include <chrono>
int64_t timedRun(const std::function<void(void)> &func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count();
}