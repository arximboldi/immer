
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

auto make_generator(std::size_t runs)
{
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<unsigned>{0, benchmark_size};
    auto r = std::vector<std::size_t>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto g = make_generator(benchmark_size);
    auto v = std::vector<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v.push_back(i);
    auto all = std::vector<std::vector<unsigned>>(meter.runs(), v);

    meter.measure([&] (int iter) {
        auto& r = all[iter];
        for (auto i = 0u; i < benchmark_size; ++i)
            r[g[i]] = i;
        return r;
    });
})

NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto g = make_generator(benchmark_size);
    auto v = rrb_create();
    for (auto i = 0u; i < benchmark_size; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < benchmark_size; ++i)
            r = rrb_update(r, g[i], reinterpret_cast<void*>(i));
        return r;
    });
    return v;
})

NONIUS_BENCHMARK("immu::vektor", [] (nonius::chronometer meter)
{
    auto g = make_generator(benchmark_size);
    auto v = immu::vektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < benchmark_size; ++i)
            r = v.assoc(g[i], i);
        return r;
    });
})

NONIUS_BENCHMARK("immu::dvektor", [] (nonius::chronometer meter)
{
    auto g = make_generator(benchmark_size);
    auto v = immu::dvektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < benchmark_size; ++i)
            r = v.assoc(g[i], i);
        return r;
    });
})
