#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/memory_policy.hpp>


struct gc_setup {
    using memory_policy = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

    static constexpr auto bits = immer::default_bits;
};

#define SETUP_T gc_setup
#include "generic.ipp"
