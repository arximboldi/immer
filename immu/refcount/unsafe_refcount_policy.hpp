
#pragma once

#include <immu/refcount/no_refcount_policy.hpp>

#include <atomic>
#include <utility>

namespace immu {

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

    template <typename Fn>
    static void dec(const data* p, Fn&& cont)
    {
        if (--p->refcount == 0)
            std::forward<Fn>(cont) ();
    };

    static void dec_unsafe(const data* p)
    {
        --p->refcount;
    };
};

} // namespace immu
