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

#include <hash_trie.hpp> // Phil Nash

#include <boost/container/flat_set.hpp>

#include <set>
#include <unordered_set>

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
auto benchmark_insert_mut_std()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g = make_generator(n);

        measure(meter, [&] {
            auto v = Set{};
            for (auto i = 0u; i < n; ++i)
                v.insert(g[i]);
            return v;
        });
    };
};

template <typename Set>
auto benchmark_insert()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g = make_generator(n);

        measure(meter, [&] {
            auto v = Set{};
            for (auto i = 0u; i < n; ++i)
                v = v.insert(g[i]);
            return v;
        });
    };
};

NONIUS_BENCHMARK("std::set", benchmark_insert_mut_std<std::set<unsigned>>())
NONIUS_BENCHMARK("std::unordered_set", benchmark_insert_mut_std<std::unordered_set<unsigned>>())
NONIUS_BENCHMARK("boost::flat_set", benchmark_insert_mut_std<boost::container::flat_set<unsigned>>())
NONIUS_BENCHMARK("hamt::hash_trie", benchmark_insert_mut_std<hamt::hash_trie<unsigned>>())

NONIUS_BENCHMARK("immer::set/5B", benchmark_insert<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,def_memory,5>>())
NONIUS_BENCHMARK("immer::set/4B", benchmark_insert<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,def_memory,4>>())
NONIUS_BENCHMARK("immer::set/GC", benchmark_insert<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,gc_memory,5>>())
NONIUS_BENCHMARK("immer::set/UN", benchmark_insert<immer::set<unsigned, std::hash<unsigned>,std::equal_to<unsigned>,unsafe_memory,5>>())
