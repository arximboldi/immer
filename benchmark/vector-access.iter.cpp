
#include <nonius/nonius_single.h++>

#include <immer/vector.hpp>
#include <immer/flex_vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

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

#if IMMER_BENCHMARK_LIBRRB
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

NONIUS_BENCHMARK("flex_vector/5B",  generic<immer::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/4B",   generic<immer::vector<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B",   generic<immer::vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B",   generic<immer::vector<unsigned,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B",  generic<immer::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B",  generic<immer::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B",  generic<immer::dvektor<unsigned,6>>())
#endif
