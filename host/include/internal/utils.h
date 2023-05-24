#pragma once

#include "common.h"

#include <functional>
#include <chrono>

#define CONCAT_2(A, B) A##_##B

#define TIMED_RUN_REGION_START_ALWAYS(var) auto CONCAT_2(start,var) = std::chrono::high_resolution_clock::now();
#define TIMED_RUN_REGION_END_ALWAYS(var) auto CONCAT_2(end,var) = std::chrono::high_resolution_clock::now(); \
var += std::chrono::duration_cast<std::chrono::microseconds>(CONCAT_2(end,var) - CONCAT_2(start,var)).count();

//#define ENABLE_TIMED_RUN

#ifdef ENABLE_TIMED_RUN

#define TIMED_RUN_REGION_START(var) TIMED_RUN_REGION_START_ALWAYS(var)
#define TIMED_RUN_REGION_END(var) TIMED_RUN_REGION_END_ALWAYS(var)

#else

#define TIMED_RUN_REGION_START(var)
#define TIMED_RUN_REGION_END(var)

#endif

#define ENABLE_PERF_LOG

#ifdef ENABLE_PERF_LOG
#define PERF_LOG(FORMAT, ...) fprintf(stderr, FORMAT"\n", __VA_ARGS__);
#endif

