
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/dvektor.hpp>
#include <immu/ivektor.hpp>

#include <vector>
#include <list>

#if IMMU_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

NONIUS_PARAM("size", std::size_t{1000})

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto benchmark_size = meter.param<std::size_t>("size");

    auto v = std::vector<unsigned>(benchmark_size);
    std::iota(v.begin(), v.end(), 0u);

    auto all = std::vector<std::vector<unsigned>>(meter.runs(), v);

    meter.measure([&] (int iter) {
        auto& r = all[iter];
        for (auto i = 0u; i < benchmark_size; ++i)
            r[i] = benchmark_size - i;
        return r;
    });
})

#if IMMU_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto benchmark_size = meter.param<std::size_t>("size");

    auto v = rrb_create();
    for (auto i = 0u; i < benchmark_size; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < benchmark_size; ++i)
            r = rrb_update(r, i, reinterpret_cast<void*>(benchmark_size - i));
        return r;
    });
})
#endif

NONIUS_BENCHMARK("immu::vektor", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    auto v = immu::vektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    return [=] {
        auto r = v;
        for (auto i = 0u; i < benchmark_size; ++i)
            r = v.assoc(i, benchmark_size - i);
        return r;
    };
})

NONIUS_BENCHMARK("immu::dvektor", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    auto v = immu::dvektor<unsigned>{};
    for (auto i = 0u; i < benchmark_size; ++i)
        v = v.push_back(i);

    return [=] {
        auto r = v;
        for (auto i = 0u; i < benchmark_size; ++i)
            r = v.assoc(i, benchmark_size - i);
        return r;
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
        auto r = v;
        for (auto i = 0u; i < benchmark_size; ++i)
            r = v.assoc(i, benchmark_size - i);
        return r;
    };
})
