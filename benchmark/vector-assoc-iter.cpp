
#include <nonius/nonius_single.h++>

#include <immer/vector.hpp>
#include <immer/array.hpp>
#include <immer/flex_vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

#include <vector>
#include <list>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

NONIUS_PARAM(N, std::size_t{1000})

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    auto all = std::vector<std::vector<unsigned>>(meter.runs(), v);

    meter.measure([&] (int iter) {
        auto& r = all[iter];
        for (auto i = 0u; i < n; ++i)
            r[i] = n - i;
        return r;
    });
})

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < n; ++i)
            r = rrb_update(r, i, reinterpret_cast<void*>(n - i));
        return r;
    });
})
#endif

template <typename Vektor,
          std::size_t Limit=std::numeric_limits<std::size_t>::max()>
auto generic()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > Limit) n = 1;

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);

        meter.measure([&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = v.assoc(i, n - i);
            return r;
        });
    };
};

using gc_memory     = immer::memory_policy<immer::heap_policy<immer::gc_heap>, immer::no_refcount_policy>;
using basic_memory  = immer::memory_policy<immer::heap_policy<immer::malloc_heap>, immer::refcount_policy>;
using unsafe_memory = immer::memory_policy<immer::default_heap_policy, immer::unsafe_refcount_policy>;

NONIUS_BENCHMARK("flex/5B",    generic<immer::flex_vector<unsigned,5>>())

NONIUS_BENCHMARK("vector/4B",  generic<immer::vector<unsigned,4>>())
NONIUS_BENCHMARK("vector/5B",  generic<immer::vector<unsigned,5>>())
NONIUS_BENCHMARK("vector/6B",  generic<immer::vector<unsigned,6>>())

NONIUS_BENCHMARK("vector/GC",  generic<immer::vector<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("vector/NO",  generic<immer::vector<unsigned,5,basic_memory>>())
NONIUS_BENCHMARK("vector/UN",  generic<immer::vector<unsigned,5,unsafe_memory>>())

#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B", generic<immer::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B", generic<immer::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B", generic<immer::dvektor<unsigned,6>>())

NONIUS_BENCHMARK("dvektor/GC", generic<immer::dvektor<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("dvektor/NO", generic<immer::dvektor<unsigned,5,basic_memory>>())
NONIUS_BENCHMARK("dvektor/UN", generic<immer::dvektor<unsigned,5,unsafe_memory>>())
#endif

NONIUS_BENCHMARK("array",    generic<immer::array<unsigned>, 10000>())
