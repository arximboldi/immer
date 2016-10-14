
#pragma once

#include <cstdlib>

namespace immer {

struct malloc_heap
{
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        return std::malloc(size);
    }

    static void deallocate(void* data)
    {
        std::free(data);
    }
};

} // namespace immer
