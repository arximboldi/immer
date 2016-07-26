
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/dvektor.hpp>
#include <vector>
#include <list>

extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}

constexpr auto benchmark_size = 1000u;

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto v = std::vector<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);
    meter.measure([&] {
        auto r = 0u;
        for (auto i = 0u; i < benchmark_size; ++i)
            r += v[i];
        volatile auto rr = r;
        return rr;
    });
})

NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto v = rrb_create();
    for (auto i = 0u; i < benchmark_size; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));
    meter.measure([&] {
        auto r = 0u;
        for (auto i = 0u; i < benchmark_size; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, i));
        volatile auto rr = r;
        return rr;
    });
    return v;
})

NONIUS_BENCHMARK("immu::vektor", [] (nonius::chronometer meter)
{
    auto v = immu::vektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);
    meter.measure([&] {
        auto r = 0u;
        for (auto i = 0u; i < benchmark_size; ++i)
            r += v[i];
        volatile auto rr = r;
        return rr;
    });
})

NONIUS_BENCHMARK("immu::dvektor", [] (nonius::chronometer meter)
{
    auto v = immu::dvektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);
    meter.measure([&] {
        auto r = 0u;
        for (auto i = 0u; i < benchmark_size; ++i)
            r += v[i];
        volatile auto rr = r;
        return rr;
    });
})
