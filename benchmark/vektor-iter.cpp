
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/dvektor.hpp>
#include <immu/ivektor.hpp>

#include <vector>
#include <list>
#include <numeric>

NONIUS_PARAM("size", std::size_t{1000})

NONIUS_BENCHMARK("std::vector", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    auto v = std::vector<unsigned>(benchmark_size);
    std::iota(v.begin(), v.end(), 0u);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

NONIUS_BENCHMARK("std::list", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    auto v = std::list<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

NONIUS_BENCHMARK("immu::vektor", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    auto v = immu::vektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

NONIUS_BENCHMARK("immu::dvektor", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    auto v = immu::dvektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})

NONIUS_BENCHMARK("immu::ivektor", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");
    if (benchmark_size > 10000) benchmark_size = 1;

    auto v = immu::ivektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    return [=] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    };
})
