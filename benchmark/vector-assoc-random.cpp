
#include <nonius/nonius_single.h++>

#include <immu/vector.hpp>
#include <immu/ivektor.hpp>
#include <immu/flex_vector.hpp>

#if IMMU_BENCHMARK_EXPERIMENTAL
#include <immu/experimental/dvektor.hpp>
#endif

#include <vector>
#include <list>

#if IMMU_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#endif

NONIUS_PARAM(N, std::size_t{1000})

auto make_generator(std::size_t runs)
{
    assert(runs > 0);
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<std::size_t>{0, runs-1};
    auto r = std::vector<std::size_t>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

NONIUS_BENCHMARK("std::vector", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto g = make_generator(n);
    auto v = std::vector<unsigned>(n);
    std::iota(v.begin(), v.end(), 0u);

    auto all = std::vector<std::vector<unsigned>>(meter.runs(), v);

    meter.measure([&] (int iter) {
        auto& r = all[iter];
        for (auto i = 0u; i < n; ++i)
            r[g[i]] = i;
        return r;
    });
})

#if IMMU_BENCHMARK_LIBRRB
NONIUS_BENCHMARK("librrb", [] (nonius::chronometer meter)
{
    auto n = meter.param<N>();

    auto g = make_generator(n);
    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i)
        v = rrb_push(v, reinterpret_cast<void*>(i));

    meter.measure([&] {
        auto r = v;
        for (auto i = 0u; i < n; ++i)
            r = rrb_update(r, g[i], reinterpret_cast<void*>(i));
        return r;
    });
})
#endif

template <typename Vektor,
          std::size_t Limit=std::numeric_limits<std::size_t>::max()>
auto generic()
{
    return [] (nonius::parameters params)
    {
        auto n = params.get<N>();
        if (n > Limit) n = 1;

        auto g = make_generator(n);
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);

        return [=] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = v.assoc(g[i], i);
            return r;
        };
    };
};

NONIUS_BENCHMARK("flex_vector/5B",  generic<immu::flex_vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/4B",   generic<immu::vector<unsigned,4>>())
NONIUS_BENCHMARK("vektor/5B",   generic<immu::vector<unsigned,5>>())
NONIUS_BENCHMARK("vektor/6B",   generic<immu::vector<unsigned,6>>())
#if IMMU_BENCHMARK_EXPERIMENTAL
NONIUS_BENCHMARK("dvektor/4B",  generic<immu::dvektor<unsigned,4>>())
NONIUS_BENCHMARK("dvektor/5B",  generic<immu::dvektor<unsigned,5>>())
NONIUS_BENCHMARK("dvektor/6B",  generic<immu::dvektor<unsigned,6>>())
#endif
NONIUS_BENCHMARK("ivektor",     generic<immu::ivektor<unsigned>, 10000>())
