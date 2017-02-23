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

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_concat()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            return v + v;
        });
    };
};

template <typename Fn>
auto benchmark_concat_librrb(Fn maker)
{
    return
        [=] (nonius::chronometer meter) {
            auto n = meter.param<N>();
            auto v = maker(n);
            measure(meter, [&] {
                    return rrb_concat(v, v);
                });
        };
}

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_concat_incr()
{
    return
        [] (nonius::chronometer meter)
        {
            auto steps = 10u;
            auto n = meter.param<N>();

            auto v = Vektor{};
            for (auto i = 0u; i < n / steps; ++i)
                v = PushFn{}(std::move(v), i);

            measure(meter, [&] {
                    auto r = v;
                    for (auto i = 0u; i < steps; ++i)
                        r = r + v;
                    return r;
                });
        };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_concat_incr_mut()
{
    return
        [] (nonius::chronometer meter)
        {
            auto steps = 10u;
            auto n = meter.param<N>();

            auto v = Vektor{};
            for (auto i = 0u; i < n / steps; ++i)
                v = PushFn{}(std::move(v), i);
            auto vv = v.transient();
            measure(meter, [&] {
                    auto r = v.transient();
                    for (auto i = 0u; i < steps; ++i)
                        r.append(vv);
                    return r;
                });
        };
};

template <typename Fn>
auto benchmark_concat_incr_librrb(Fn maker)
{
    return
        [=] (nonius::chronometer meter) {
            auto n = meter.param<N>();
            auto steps = 10u;
            auto v = maker(n / steps);
            measure(meter, [&] {
                    auto r = v;
                    for (auto i = 0ul; i < steps; ++i)
                        r = rrb_concat(r, v);
                    return r;
                });
        };
}

} // anonymous namespace
