//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#include "benchmark/config.hpp"

#include <immer/set.hpp>
#include <immer/algorithm.hpp>

#include <hash_trie.hpp> // Phil Nash

#include <boost/container/flat_set.hpp>

#include <set>
#include <unordered_set>
#include <numeric>

template <typename T=unsigned>
auto make_generator(std::size_t runs)
{
    assert(runs > 0);
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<T>{};
    auto r = std::vector<T>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

template <typename Set>
auto benchmark_access_std_iter()
{
    return [] (nonius::chronometer meter)
    {
        auto n  = meter.param<N>();
        auto g1 = make_generator(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v.insert(g1[i]);

        measure(meter, [&] {
            volatile auto c = std::accumulate(v.begin(), v.end(), 0u);
            return c;
        });
    };
};

template <typename Set>
auto benchmark_access_reduce()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = make_generator(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(g1[i]);

        measure(meter, [&] {
            volatile auto c = immer::accumulate(v, 0u);
            return c;
        });
    };
};

template <typename Set>
auto benchmark_access_iter()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = make_generator(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(g1[i]);

        measure(meter, [&] {
            volatile auto c = std::accumulate(v.begin(), v.end(), 0u);
            return c;
        });
    };
};

NONIUS_BENCHMARK("iter/std::set", benchmark_access_std_iter<std::set<unsigned>>())
NONIUS_BENCHMARK("iter/std::unordered_set", benchmark_access_std_iter<std::unordered_set<unsigned>>())
NONIUS_BENCHMARK("iter/boost::flat_set", benchmark_access_std_iter<boost::container::flat_set<unsigned>>())
NONIUS_BENCHMARK("iter/hamt::hash_trie", benchmark_access_std_iter<hamt::hash_trie<unsigned>>())
NONIUS_BENCHMARK("iter/immer::set/5B", benchmark_access_iter<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,def_memory,5>>())
NONIUS_BENCHMARK("iter/immer::set/4B", benchmark_access_iter<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,def_memory,4>>())

NONIUS_BENCHMARK("reduce/immer::set/5B", benchmark_access_reduce<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,def_memory,5>>())
NONIUS_BENCHMARK("reduce/immer::set/4B", benchmark_access_reduce<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,def_memory,4>>())
