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
#include <hash_trie.hpp> // Phil Nash
#include <boost/container/flat_set.hpp>
#include <set>
#include <unordered_set>

namespace {

template <typename Generator, typename Set>
auto benchmark_insert_mut_std()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g = Generator{}(n);

        measure(meter, [&] {
            auto v = Set{};
            for (auto i = 0u; i < n; ++i)
                v.insert(g[i]);
            return v;
        });
    };
}

template <typename Generator, typename Set>
auto benchmark_insert()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto g = Generator{}(n);

        measure(meter, [&] {
            auto v = Set{};
            for (auto i = 0u; i < n; ++i)
                v = v.insert(g[i]);
            return v;
        });
    };
}

} // namespace
