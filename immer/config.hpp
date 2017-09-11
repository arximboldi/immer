//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#ifndef IMMER_DEBUG_TRACES
#define IMMER_DEBUG_TRACES 0
#endif

#ifndef IMMER_DEBUG_PRINT
#define IMMER_DEBUG_PRINT 0
#endif

#ifndef IMMER_DEBUG_DEEP_CHECK
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
#define IMMER_TRACE_F(...)                                              \
    IMMER_TRACE(__FILE__ << ":" << __LINE__ << ": " << __VA_ARGS__)
#define IMMER_TRACE_E(expr)                             \
    IMMER_TRACE("    " << #expr << " = " << (expr))

#define IMMER_UNREACHABLE    __builtin_unreachable()
#define IMMER_LIKELY(cond)   __builtin_expect(!!(cond), 1)
#define IMMER_UNLIKELY(cond) __builtin_expect(!!(cond), 0)
// #define IMMER_PREFETCH(p)    __builtin_prefetch(p)
#define IMMER_PREFETCH(p)
#define IMMER_FORCEINLINE    inline __attribute__ ((always_inline))

#define IMMER_DESCENT_DEEP 0

#ifdef NDEBUG
#define IMMER_ENABLE_DEBUG_SIZE_HEAP 0
#else
#define IMMER_ENABLE_DEBUG_SIZE_HEAP 1
#endif

namespace immer {

const auto default_bits = 5;
const auto default_free_list_size = 1 << 10;

} // namespace immer
