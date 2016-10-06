
#pragma once

#include <cstdlib>

namespace immer {

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
};

} // namespace immer
