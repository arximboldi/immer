
#include <nonius/nonius_single.h++>

#include <immer/vector.hpp>
#include <immer/flex_vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <vector>
#include <list>
#include <random>
#include <functional>

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

NONIUS_PARAM(N, std::size_t{1000})

auto make_generator(std::size_t runs)
{
    assert(runs > 0);
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<std::size_t>{0, runs-1};
    auto r = std::vector<std::size_t>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

NONIUS_BENCHMARK("std::vector", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto g = make_generator(n);
    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    return [=] {
        volatile auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += v[g[i]];
        return r;
    };
})

#if IMMER_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));
    auto g = make_generator(n);

    return [=] {
        volatile auto r = 0u;
        for (auto i = 0u; i < n; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, g[i]));
        return r;
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
        if (n > Limit) n = 0;

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);
        auto g = make_generator(n);

        return [=] {
            volatile auto r = 0u;
            for (auto i = 0u; i < n; ++i)
                r += v[g[i]];
            return r;
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
