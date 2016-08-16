
#pragma once

#include <immu/refcount/no_refcount_policy.hpp>

#include <atomic>
#include <utility>

namespace immu {

struct refcount_policy
{
    static constexpr bool enabled = true;

    struct data
    {
        mutable std::atomic<int> refcount;

        data() : refcount{1} {};
        data(disowned) : refcount{0} {}
    };

    static void inc(const data* p)
    {
        p->refcount.fetch_add(1, std::memory_order_relaxed);
    };

    template <typename Fn>
    static void dec(const data* p, Fn&& cont)
    {
        if (1 == p->refcount.fetch_sub(1, std::memory_order_release)) {
            std::atomic_thread_fence(std::memory_order_acquire);
            std::forward<Fn>(cont) ();
        }
    };

    template <typename T>
    static void dec_unsafe(const T* p)
    {
        assert(p->refcount.load() > 1);
        p->refcount.fetch_sub(1, std::memory_order_relaxed);
    };
};

} // namespace immu
