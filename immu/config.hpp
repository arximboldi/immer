
#pragma once

#ifdef NDEBUG
#define IMMU_TRACE_ON 0
#else
#define IMMU_TRACE_ON 1
#endif

#if IMMU_TRACE_ON
#include <iostream>
#include <prettyprint.hpp>
#define IMMU_TRACE(...) std::cout << __VA_ARGS__ << std::endl
#else
#define IMMU_TRACE(...)
#endif
