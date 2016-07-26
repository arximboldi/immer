
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/dvektor.hpp>
#include <vector>
#include <list>
#include <random>

extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}

constexpr auto benchmark_size = 1000u;

auto make_generator()
{
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<unsigned>{0, benchmark_size};
    return std::bind(dist, engine);
}

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto v = std::vector<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);
    meter.measure([&] {
        auto r = 0u;
        auto g = make_generator();
        for (auto i = 0u; i < benchmark_size; ++i)
            r += v[g()];
        return r;
    });
})

NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto v = rrb_create();
    for (auto i = 0u; i < benchmark_size; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));
    meter.measure([&] {
        auto r = 0u;
        auto g = make_generator();
        for (auto i = 0u; i < benchmark_size; ++i)
            r += reinterpret_cast<unsigned long>(rrb_nth(v, g()));
        return r;
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
        auto g = make_generator();
        for (auto i = 0u; i < benchmark_size; ++i)
            r += v[g()];
        return r;
    });
})

NONIUS_BENCHMARK("immu::dvektor", [] (nonius::chronometer meter)
{
    auto v = immu::dvektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);
    meter.measure([&] {
        auto r = 0u;
        auto g = make_generator();
        for (auto i = 0u; i < benchmark_size; ++i)
            r += v[g()];
        return r;
    });
})
