
#pragma once

#define IMMU_DEBUG_TRACE_0N 0

#ifdef NDEBUG
#define IMMU_TRACE_ON 0
#define IMMU_DEBUG_DEEP_CHECK 0
#else
#define IMMU_TRACE_ON IMMU_DEBUG_TRACE_0N
#define IMMU_DEBUG_DEEP_CHECK 0
#endif

#if IMMU_TRACE_ON
#include <iostream>
#include <prettyprint.hpp>
#define IMMU_TRACE(...) std::cout << __VA_ARGS__ << std::endl
#else
#define IMMU_TRACE(...)
#endif

#define IMMU_FAST_HEAP 0
