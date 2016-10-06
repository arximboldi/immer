
#pragma once

#include <immer/heap/heap_policy.hpp>
#include <immer/refcount/refcount_policy.hpp>

namespace immer {

template <typename HeapPolicy,
          typename RefcountPolicy>
struct memory_policy
{
    using heap     = HeapPolicy;
    using refcount = RefcountPolicy;
};

using default_memory_policy = memory_policy<
    default_heap_policy,
    refcount_policy>;

} // namespace immer
