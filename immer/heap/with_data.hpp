#pragma once

#include <cstdio>

namespace immer {

template <typename T, typename Base>
struct with_data : Base
{
    using base_t = Base;

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags... tags)
    {
        auto p = base_t::allocate(size + sizeof(T), tags...);
        return new (p) T{} + 1;
    }

    template <typename... Tags>
    static void deallocate(void* p, Tags... tags)
    {
        auto dp = static_cast<T*>(p) - 1;
        dp->~T();
        base_t::deallocate(dp, tags...);
    }
};

} // namespace immer
