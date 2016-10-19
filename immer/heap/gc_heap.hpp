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

#include <immer/heap/tags.hpp>

#if IMMER_HAS_LIBGC
#include <gc/gc.h>
#else
#error "Using garbage collection requires libgc"
#endif

#include <cstdlib>

namespace immer {

struct gc_heap
{
    static void* allocate(std::size_t n)
    {
        return GC_malloc(n);
    }

    static void* allocate(std::size_t n, norefs_tag)
    {
        return GC_malloc_atomic(n);
    }

    static void deallocate(void* data)
    {
        GC_free(data);
    }

    static void deallocate(void* data, norefs_tag)
    {
        GC_free(data);
    }
};

} // namespace immer
