
#pragma once

#include <immu/heap/heap_policy.hpp>
#include <immu/refcount/refcount_policy.hpp>

namespace immu {

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

} // namespace immu
