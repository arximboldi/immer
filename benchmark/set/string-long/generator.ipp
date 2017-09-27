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

#include <random>
#include <vector>
#include <cassert>
#include <functional>
#include <algorithm>

#define GENERATOR_T generate_unsigned

namespace {

struct GENERATOR_T
{
    static constexpr auto char_set   = "_-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static constexpr auto max_length = 256;
    static constexpr auto min_length = 32;

    auto operator() (std::size_t runs) const
    {
        assert(runs > 0);
        auto engine = std::default_random_engine{42};
        auto dist = std::uniform_int_distribution<unsigned>{};
        auto gen = std::bind(dist, engine);
        auto r = std::vector<std::string>(runs);
        std::generate_n(r.begin(), runs, [&] {
            auto len = gen() % (max_length - min_length) + min_length;
            auto str = std::string(len, ' ');
            std::generate_n(str.begin(), len, [&] {
                return char_set[gen() % sizeof(char_set)];
            });
            return str;
        });
        return r;
    }
};

} // namespace
