
#include <nonius/nonius_single.h++>

#include "util.hpp"

#include <immer/flex_vector.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

NONIUS_PARAM(N, std::size_t{1000})

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    meter.measure([&] {
        for (auto i = 0u; i < n; ++i)
            rrb_slice(v, i, n);
    });
});

NONIUS_BENCHMARK("librrb/F", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        auto f = rrb_push(rrb_create(),
                          reinterpret_cast<void*>(i));
        v = rrb_concat(f, v);
    }

    meter.measure([&] {
        for (auto i = 0u; i < n; ++i)
            rrb_slice(v, i, n);
    });
})
#endif

template <typename Vektor,
          typename PushFn=push_back_fn>
auto generic()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        meter.measure([&] {
            for (auto i = 0u; i < n; ++i)
                v.drop(i);
        });
    };
};

using gc_memory     = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy>;
using basic_memory  = immer::memory_policy<immer::heap_policy<immer::malloc_heap>, immer::refcount_policy>;
using unsafe_memory = immer::memory_policy<immer::default_heap_policy, immer::unsafe_refcount_policy>;

NONIUS_BENCHMARK("flex/4B", generic<immer::flex_vector<unsigned,4>>())
NONIUS_BENCHMARK("flex/5B", generic<immer::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("flex/6B", generic<immer::flex_vector<unsigned,6>>())
NONIUS_BENCHMARK("flex/GC", generic<immer::flex_vector<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("flex/NO", generic<immer::flex_vector<unsigned,5,basic_memory>>())
NONIUS_BENCHMARK("flex/UN", generic<immer::flex_vector<unsigned,5,unsafe_memory>>())

NONIUS_BENCHMARK("flex/F/5B", generic<immer::flex_vector<unsigned,5>, push_front_fn>())
NONIUS_BENCHMARK("flex/F/GC", generic<immer::flex_vector<unsigned,5,gc_memory>, push_front_fn>())
