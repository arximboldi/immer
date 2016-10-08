
#pragma once

#ifdef NDEBUG
#define IMMER_DEBUG_TRACES 0
#define IMMER_DEBUG_PRINT 0
#define IMMER_DEBUG_DEEP_CHECK 0
#else
#define IMMER_DEBUG_TRACES 0
#define IMMER_DEBUG_PRINT 0
#define IMMER_DEBUG_DEEP_CHECK 0
#endif

#if IMMER_DEBUG_TRACES || IMMER_DEBUG_PRINT
#include <iostream>
#include <prettyprint.hpp>
#endif

#if IMMER_DEBUG_TRACES
#define IMMER_TRACE(...) std::cout << __VA_ARGS__ << std::endl
#else
#define IMMER_TRACE(...)
#endif
