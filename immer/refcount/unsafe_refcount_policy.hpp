
#pragma once

#include <immer/refcount/no_refcount_policy.hpp>

#include <atomic>
#include <utility>

namespace immer {

struct unsafe_refcount_policy
{
    static constexpr bool refcounting = true;

    struct data
    {
        mutable int refcount;

        data() : refcount{1} {};
        data(disowned) : refcount{0} {}
    };

    static void inc(const data* p)
    {
        ++p->refcount;
    };

    static bool dec(const data* p)
    {
        return --p->refcount == 0;
    };

    static void dec_unsafe(const data* p)
    {
        --p->refcount;
    };
};

} // namespace immer
