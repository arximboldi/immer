
#pragma once

#if IMMU_HAS_LIBGC
#include <gc/gc.h>
#else
#error "Using garbage collection requires libgc"
#endif

#include <cstdlib>

namespace immu {

struct gc_heap
{
    static void* allocate(std::size_t n)
    {
        return GC_malloc(n);
    }

    static void deallocate(void* data)
    {
        GC_free(data);
    }
};

} // namespace immu
