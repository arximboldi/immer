
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/ivektor.hpp>
#include <immu/rvektor.hpp>

#if IMMU_BENCHMARK_EXPERIMENTAL
#include <immu/experimental/dvektor.hpp>
#endif

#include <immu/heap/gc_heap.hpp>
#include <immu/refcount/no_refcount_policy.hpp>
#include <immu/refcount/unsafe_refcount_policy.hpp>

#include <vector>
#include <list>

#if IMMU_BENCHMARK_LIBRRB
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

#if IMMU_BENCHMARK_LIBRRB
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

using gc_memory     = immu::memory_policy<immu::heap_policy<immu::gc_heap>, immu::no_refcount_policy>;
using basic_memory  = immu::memory_policy<immu::heap_policy<immu::malloc_heap>, immu::refcount_policy>;
using unsafe_memory = immu::memory_policy<immu::default_heap_policy, immu::unsafe_refcount_policy>;

NONIUS_BENCHMARK("rvektor/5B", generic<immu::rvektor<unsigned,5>>())

NONIUS_BENCHMARK("vektor/4B",  generic<immu::vektor<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B",  generic<immu::vektor<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B",  generic<immu::vektor<unsigned,6>>())

NONIUS_BENCHMARK("vektor/GC",  generic<immu::vektor<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("vektor/NO",  generic<immu::vektor<unsigned,5,basic_memory>>())
NONIUS_BENCHMARK("vektor/UN",  generic<immu::vektor<unsigned,5,unsafe_memory>>())

#if IMMU_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B", generic<immu::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B", generic<immu::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B", generic<immu::dvektor<unsigned,6>>())

NONIUS_BENCHMARK("dvektor/GC", generic<immu::dvektor<unsigned,5,gc_memory>>())
NONIUS_BENCHMARK("dvektor/NO", generic<immu::dvektor<unsigned,5,basic_memory>>())
NONIUS_BENCHMARK("dvektor/UN", generic<immu::dvektor<unsigned,5,unsafe_memory>>())
#endif

NONIUS_BENCHMARK("ivektor",    generic<immu::ivektor<unsigned>, 10000>())
