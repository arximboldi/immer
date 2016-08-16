
#pragma once

#include <cstdlib>

namespace immu {
namespace detail {

struct malloc_heap
{
    static void* allocate(std::size_t size)
    {
        return std::malloc(size);
    }

    static void deallocate(void* data)
    {
        std::free(data);
    }

    template <typename Fn>
    static void set_finalizer(void*, Fn&&)
    {
    }
};

} // namespace detail
} // namespace immu
