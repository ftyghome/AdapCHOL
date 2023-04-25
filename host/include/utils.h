#pragma once
#include <functional>
int64_t timedRun(const std::function<void(void)> &func);