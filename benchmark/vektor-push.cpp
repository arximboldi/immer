
#include <nonius/nonius_single.h++>

#include <immu/vektor.hpp>
#include <immu/dvektor.hpp>
#include <immu/ivektor.hpp>

#include <vector>
#include <list>
#include <numeric>

#if IMMU_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

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

#if IMMU_BENCHMARK_LIBRRB
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
#endif

template <typename Vektor,
          std::size_t Limit=std::numeric_limits<std::size_t>::max()>
auto generic()
{
    return [] (nonius::parameters params)
    {
        auto benchmark_size = params.get<std::size_t>("size");
        if (benchmark_size > Limit) benchmark_size = 1;

        return [=] {
            auto v = Vektor{};
            for (auto i = 0u; i < benchmark_size; ++i)
                v = v.push_back(i);
            return v;
        };
    };
};

NONIUS_BENCHMARK("immu::vektor/4B",   generic<immu::vektor<unsigned,4>>())
NONIUS_BENCHMARK("immu::vektor/5B",   generic<immu::vektor<unsigned,5>>())
NONIUS_BENCHMARK("immu::vektor/6B",   generic<immu::vektor<unsigned,6>>())
NONIUS_BENCHMARK("immu::dvektor/4B",  generic<immu::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("immu::dvektor/5B",  generic<immu::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("immu::dvektor/6B",  generic<immu::dvektor<unsigned,6>>())
NONIUS_BENCHMARK("immu::ivektor",     generic<immu::ivektor<unsigned>, 10000>())
