
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/dvektor.hpp>
#include <vector>
#include <list>
#include <numeric>

constexpr auto benchmark_size = 1000u;

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto v = std::vector<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);

    meter.measure([&v] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    });
})

NONIUS_BENCHMARK("std::list", [] (nonius::chronometer meter)
{
    auto v = std::list<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);

    meter.measure([&v] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    });
})

NONIUS_BENCHMARK("immu::vektor", [] (nonius::chronometer meter)
{
    auto v = immu::vektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    meter.measure([&v] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    });
})

NONIUS_BENCHMARK("immu::dvektor", [] (nonius::chronometer meter)
{
    auto v = immu::dvektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    meter.measure([&v] {
        auto volatile x = std::accumulate(v.begin(), v.end(), 0u);
        return x;
    });
})
