
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/dvektor.hpp>
#include <immu/ivektor.hpp>

#include <vector>
#include <list>
#include <numeric>

extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}

NONIUS_PARAM("size", std::size_t{1000})

NONIUS_BENCHMARK("std::vector", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    return [=] {
        auto v = std::vector<unsigned>{};
        for (auto i = 0u; i < benchmark_size; ++i)
            v.push_back(i);
        return v;
    };
})

NONIUS_BENCHMARK("std::list", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    return [=] {
        auto v = std::list<unsigned>{};
        for (auto i = 0u; i < benchmark_size; ++i)
            v.push_back(i);
        return v;
    };
})

NONIUS_BENCHMARK("librrb", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    return [=] {
        auto v = rrb_create();
        for (auto i = 0u; i < benchmark_size; ++i)
            v = rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    };
})

NONIUS_BENCHMARK("immu::vektor", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    return [=] {
        auto v = immu::vektor<unsigned>{};
        for (auto i = 0u; i < benchmark_size; ++i)
            v = v.push_back(i);
        return v;
    };
})

NONIUS_BENCHMARK("immu::dvektor", [] (nonius::parameters params)
{
    auto benchmark_size = params.get<std::size_t>("size");

    return [=] {
        auto v = immu::dvektor<unsigned>{};
        for (auto i = 0u; i < benchmark_size; ++i)
            v = v.push_back(i);
        return v;
    };
})

NONIUS_BENCHMARK("immu::ivektor", [] (nonius::parameters params)
{
    auto benchmark_size = std::min(params.get<std::size_t>("size"), std::size_t{10000});
    if (benchmark_size > 10000) benchmark_size = 1;

    return [=] {
        auto v = immu::ivektor<unsigned>{};
        for (auto i = 0u; i < benchmark_size; ++i)
            v = v.push_back(i);
        return v;
    };
})
