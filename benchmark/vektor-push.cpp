
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <vector>
#include <list>
#include <numeric>

extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}

constexpr auto benchmark_size = 1000u;

NONIUS_BENCHMARK("std::vector", []
{
    auto v = std::vector<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);
    return v;
})

NONIUS_BENCHMARK("std::list", []
{
    auto v = std::list<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);
    return v;
})

NONIUS_BENCHMARK("librrb", []
{
    auto v = rrb_create();
    for (auto i = 0u; i < benchmark_size; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));
    return v;
})

NONIUS_BENCHMARK("immu::vektor", []
{
    auto v = immu::vektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);
    return v;
})
