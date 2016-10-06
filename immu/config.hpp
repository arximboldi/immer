
#pragma once

#ifdef NDEBUG
#define IMMU_DEBUG_TRACES 0
#define IMMU_DEBUG_PRINT 0
#define IMMU_DEBUG_DEEP_CHECK 0
#else
#define IMMU_DEBUG_TRACES 0
#define IMMU_DEBUG_PRINT 0
#define IMMU_DEBUG_DEEP_CHECK 0
#endif

#if IMMU_DEBUG_TRACES || IMMU_DEBUG_PRINT
#include <iostream>
#include <prettyprint.hpp>
#endif

#if IMMU_DEBUG_TRACES
#define IMMU_TRACE(...) std::cout << __VA_ARGS__ << std::endl
#else
#define IMMU_TRACE(...)
#endif

#define IMMU_FAST_HEAP 1
