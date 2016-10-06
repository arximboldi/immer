
#include <nonius/nonius_single.h++>

#include <immu/vector.hpp>
#include <immu/ivektor.hpp>
#include <immu/rvektor.hpp>

#if IMMU_BENCHMARK_EXPERIMENTAL
#include <immu/experimental/dvektor.hpp>
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

NONIUS_BENCHMARK("vektor/4B",   generic<immu::vector<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B",   generic<immu::vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B",   generic<immu::vector<unsigned,6>>())
#if IMMU_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B",  generic<immu::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B",  generic<immu::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B",  generic<immu::dvektor<unsigned,6>>())
#endif
NONIUS_BENCHMARK("ivektor",     generic<immu::ivektor<unsigned>, 10000>())

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

NONIUS_BENCHMARK("rvektor/5B/reduce",  reduce_generic<immu::rvektor<unsigned,5>>())
NONIUS_BENCHMARK("vektor/4B/reduce",   reduce_generic<immu::vector<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B/reduce",   reduce_generic<immu::vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B/reduce",   reduce_generic<immu::vector<unsigned,6>>())
