
#pragma once

#include <cstdlib>

namespace immu {
namespace detail {

template <typename Heap>
struct enable_heap
{
    using heap_type = Heap;

    static void* operator new (std::size_t size)
    {
        return heap_type::allocate(size);
    }

    static void operator delete (void* data)
    {
        heap_type::deallocate(data);
    }
};

} // namespace detail
} // namespace immu
