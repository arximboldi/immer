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

#pragma once

#include "benchmark/config.hpp"

#include <immer/set.hpp>
#include <immer/box.hpp>
#include <immer/algorithm.hpp>
#include <hash_trie.hpp> // Phil Nash
#include <boost/container/flat_set.hpp>
#include <set>
#include <unordered_set>
#include <numeric>

namespace {

template <typename T>
struct iter_step
{
    unsigned operator() (unsigned x, const T& y) const
    {
        return x + y;
    }
};

template <>
struct iter_step<std::string>
{
    unsigned operator() (unsigned x, const std::string& y) const
    {
        return x + (unsigned) y.size();
    }
};

template <>
struct iter_step<immer::box<std::string>>
{
    unsigned operator() (unsigned x, const immer::box<std::string>& y) const
    {
        return x + (unsigned) y->size();
    }
};

template <typename Generator, typename Set>
auto benchmark_access_std_iter()
{
    return [] (nonius::chronometer meter)
    {
        auto n  = meter.param<N>();
        auto g1 = Generator{}(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v.insert(g1[i]);

        using step_t = iter_step<typename decltype(g1)::value_type>;
        measure(meter, [&] {
            volatile auto c = std::accumulate(v.begin(), v.end(), 0u, step_t{});
            return c;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_access_reduce()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = Generator{}(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(g1[i]);

        using step_t = iter_step<typename decltype(g1)::value_type>;
        measure(meter, [&] {
            volatile auto c = immer::accumulate(v, 0u, step_t{});
            return c;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_access_iter()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g1 = Generator{}(n);

        auto v = Set{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(g1[i]);

        using step_t = iter_step<typename decltype(g1)::value_type>;
        measure(meter, [&] {
            volatile auto c = std::accumulate(v.begin(), v.end(), 0u, step_t{});
            return c;
        });
    };
}

} // namespace
