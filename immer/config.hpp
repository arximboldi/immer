//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
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

#define IMMER_TRACE_F(...)                                              \
    IMMER_TRACE(__FILE__ << ":" << __LINE__ << ": " << __VA_ARGS__)
#define IMMER_TRACE_E(expr)                             \
    IMMER_TRACE("    " << #expr << " = " << (expr))

namespace immer {

const auto default_bits = 5;

} // namespace immer
