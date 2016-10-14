
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

    static void* allocate(std::size_t n, nowrite_tag)
    {
        return GC_malloc_stubborn(n);
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

    static void write(void* p)
    {
        GC_change_stubborn(p);
    }

    static void end_write(void* p)
    {
        GC_end_stubborn_change(p);
    }
};

} // namespace immer
