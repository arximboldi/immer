//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include "benchmark/vector/common.hpp"

namespace {

template <typename Vektor>
auto bechmark_push_mut()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v.push_back(i);
            return v;
        });
    };
};

template <typename Vektor>
auto bechmark_push_move()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = std::move(v).push_back(i);
            return v;
        });
    };
};

template <typename Vektor>
auto benchmark_push()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        measure(meter, [&] {
            auto v = Vektor{};
            for (auto i = 0u; i < n; ++i)
                v = v.push_back(i);
            return v;
        });
    };
};

auto benchmark_push_librrb(nonius::chronometer meter)
{
    auto n = meter.param<N>();

    measure(meter, [&] {
        auto v = rrb_create();
        for (auto i = 0u; i < n; ++i)
            v = rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    });
}

auto benchmark_push_mut_librrb(nonius::chronometer meter)
{
    auto n = meter.param<N>();

    measure(meter, [&] {
        auto v = rrb_create();
        for (auto i = 0u; i < n; ++i)
            v = rrb_push(v, reinterpret_cast<void*>(i));
        return v;
    });
}

} // anonymous namespace
