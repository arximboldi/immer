
#include <nonius/nonius_single.h++>

#include <immer/vector.hpp>
#include <immer/array.hpp>
#include <immer/flex_vector.hpp>

#if IMMER_BENCHMARK_EXPERIMENTAL
#include <immer/experimental/dvektor.hpp>
#endif

#include <vector>
#include <list>
#include <numeric>

NONIUS_PARAM(N, std::size_t{1000})

NONIUS_BENCHMARK("std::vector", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

NONIUS_BENCHMARK("std::list", [] (nonius::parameters params)
{
    auto n = params.get<N>();

    auto v = std::list<unsigned>{};
    for (auto i = 0u; i < n; ++i)
        v.push_back(i);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

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
            auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
            return x;
        };
    };
}

NONIUS_BENCHMARK("vektor/4B",   generic<immer::vector<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B",   generic<immer::vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B",   generic<immer::vector<unsigned,6>>())
#if IMMER_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B",  generic<immer::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B",  generic<immer::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B",  generic<immer::dvektor<unsigned,6>>())
#endif
NONIUS_BENCHMARK("array",     generic<immer::array<unsigned>, 10000>())

template <typename Vektor,
          std::size_t Limit=std::numeric_limits<std::size_t>::max()>
auto reduce_generic()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        if (n > Limit) n = 1;

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);

        return [=] {
            auto volatile x = v.reduce(std::plus<unsigned>{}, 0u);
            return x;
        };
    };
}

NONIUS_BENCHMARK("flex_vector/5B/reduce",  reduce_generic<immer::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/4B/reduce",   reduce_generic<immer::vector<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B/reduce",   reduce_generic<immer::vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B/reduce",   reduce_generic<immer::vector<unsigned,6>>())
