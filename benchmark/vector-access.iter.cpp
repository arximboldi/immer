
#include <nonius/nonius_single.h++>

#include <immu/vector.hpp>
#include <immu/flex_vector.hpp>

#if IMMU_BENCHMARK_EXPERIMENTAL
#include <immu/experimental/dvektor.hpp>
#endif

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

NONIUS_BENCHMARK("std::vector", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    return [=] {
        auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += v[i];
        volatile auto rr = r;
        return rr;
    };
})

#if IMMU_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    return [=] {
        auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, i));
        volatile auto rr = r;
        return rr;
    };
})
#endif

template <typename Vektor,
          std::size_t Limit=std::numeric_limits<std::size_t>::max()>
auto generic()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        if (n > Limit) n = 1;

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);

        return [=] {
            auto r = 0u;
            for (auto i = 0u; i < n; ++i)
                r += v[i];
            volatile auto rr = r;
            return rr;
        };
    };
};

NONIUS_BENCHMARK("flex_vector/5B",  generic<immu::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/4B",   generic<immu::vector<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B",   generic<immu::vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B",   generic<immu::vector<unsigned,6>>())
#if IMMU_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B",  generic<immu::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B",  generic<immu::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B",  generic<immu::dvektor<unsigned,6>>())
#endif
