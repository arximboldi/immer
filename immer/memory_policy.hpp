
#pragma once

#include <immer/heap/heap_policy.hpp>
#include <immer/refcount/refcount_policy.hpp>
#include <type_traits>

namespace immer {

template <typename HeapPolicy,
          typename RefcountPolicy,
          bool PreferFewerBiggerObjects = !std::is_same<
              HeapPolicy,
              free_list_heap_policy>()>
struct memory_policy
{
    using heap     = HeapPolicy;
    using refcount = RefcountPolicy;

    static constexpr bool prefer_fewer_bigger_objects =
        PreferFewerBiggerObjects;
};

#if IMMER_FREE_LIST
using default_heap_policy = free_list_heap_policy;
#else
using default_heap_policy = heap_policy<malloc_heap>;
#endif

using default_refcount_policy = refcount_policy;

using default_memory_policy = memory_policy<
    default_heap_policy,
    default_refcount_policy>;

} // namespace immer
