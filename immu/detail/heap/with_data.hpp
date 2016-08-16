#pragma once

#include <cstdio>

namespace immu {
namespace detail {

template <typename T, typename Base>
struct with_data : Base
{
    using base_t = Base;

    static void* allocate(std::size_t size)
    {
        auto p = base_t::allocate(size + sizeof(T));
        return new (p) T{} + 1;
    }

    static void deallocate(void* p)
    {
        auto dp = static_cast<T*>(p) - 1;
        dp->~T();
        base_t::deallocate(dp);
    }
};

} // namespace detail
} // namespace immu
