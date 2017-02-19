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
auto benchmark_take()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            for (auto i = 0u; i < n; ++i)
                v.take(i);
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_take_lin()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = n; i > 0; --i)
                r = r.take(i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_take_move()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = n; i > 0; --i)
                r = std::move(r).take(i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_take_mut()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();

        auto vv = Vektor{};
        for (auto i = 0u; i < n; ++i)
            vv = PushFn{}(std::move(vv), i);

        measure(meter, [&] {
            auto v = vv.transient();
            for (auto i = n; i > 0; --i)
                v.take(i);
        });
    };
};

template <typename Fn>
auto benchmark_take_librrb(Fn make)
{
    return [=] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto v = make(n);
        measure(meter, [&] {
                for (auto i = 0u; i < n; ++i)
                    rrb_slice(v, 0, i);
            });
    };
}

template <typename Fn>
auto benchmark_take_lin_librrb(Fn make)
{
    return [=] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto v = make(n);
        measure(
            meter, [&] {
                auto r = v;
                for (auto i = n; i > 0; --i)
                    r = rrb_slice(r, 0, i);
                return r;
            });
    };
}

template <typename Fn>
auto benchmark_take_mut_librrb(Fn make)
{
    return [=] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        auto v = make(n);
        measure(
            meter, [&] {
                auto r = rrb_to_transient(v);
                for (auto i = n; i > 0; --i)
                    r = transient_rrb_slice(r, 0, i);
                return r;
            });
    };
}

} // anonymous namespace
