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
auto benchmark_assoc_std()
{
    return
        [] (nonius::chronometer meter)
        {
            auto n = meter.param<N>();
            auto v = Vektor(n);
            std::iota(v.begin(), v.end(), 0u);
            auto all = std::vector<Vektor>(meter.runs(), v);
            meter.measure([&] (int iter) {
                auto& r = all[iter];
                for (auto i = 0u; i < n; ++i)
                    r[i] = n - i;
                return r;
            });
        };
}

template <typename Vektor>
auto benchmark_assoc_random_std()
{
    return
        [](nonius::chronometer meter)
        {
            auto n = meter.param<N>();
            auto g = make_generator(n);
            auto v = Vektor(n);
            std::iota(v.begin(), v.end(), 0u);
            auto all = std::vector<Vektor>(meter.runs(), v);
            meter.measure([&] (int iter) {
                auto& r = all[iter];
                for (auto i = 0u; i < n; ++i)
                    r[g[i]] = n - i;
                return r;
            });
        };
}

template <typename Vektor,
          typename PushFn=push_back_fn,
          typename SetFn=set_fn>
auto benchmark_assoc()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = SetFn{}(r, i, n - i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_assoc_move()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = std::move(r).set(i, n - i);
            return r;
        });
    };
};

template <typename Vektor,
          typename SetFn=set_fn>
auto benchmark_assoc_random()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto g = make_generator(n);
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = v.push_back(i);

        measure(meter, [&] {
            auto r = v;
            for (auto i = 0u; i < n; ++i)
                r = SetFn{}(r, g[i], i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_assoc_mut()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v.transient();
            for (auto i = 0u; i < n; ++i)
                r.set(i, n - i);
            return r;
        });
    };
};

template <typename Vektor,
          typename PushFn=push_back_fn>
auto benchmark_assoc_mut_random()
{
    return [] (nonius::chronometer meter)
    {
        auto n = meter.param<N>();
        if (n > get_limit<Vektor>{})
            nonius::skip();

        auto g = make_generator(n);
        auto v = Vektor{};
        for (auto i = 0u; i < n; ++i)
            v = PushFn{}(std::move(v), i);

        measure(meter, [&] {
            auto r = v.transient();
            for (auto i = 0u; i < n; ++i)
                r.set(g[i], i);
            return r;
        });
    };
};

template <typename Fn>
auto benchmark_assoc_librrb(Fn maker)
{
    return
        [=] (nonius::chronometer meter) {
            auto n = meter.param<N>();
            auto v = maker(n);
            measure(
                meter, [&] {
                    auto r = v;
                    for (auto i = 0u; i < n; ++i)
                        r = rrb_update(r, i, reinterpret_cast<void*>(n - i));
                    return r;
                });
        };
}

template <typename Fn>
auto benchmark_assoc_random_librrb(Fn maker)
{
    return
        [=] (nonius::chronometer meter) {
            auto n = meter.param<N>();
            auto v = maker(n);
            auto g = make_generator(n);
            measure(
                meter, [&] {
                    auto r = v;
                    for (auto i = 0u; i < n; ++i)
                        r = rrb_update(r, g[i], reinterpret_cast<void*>(i));
                    return r;
                });
        };
}

template <typename Fn>
auto benchmark_assoc_mut_librrb(Fn maker)
{
    return
        [=] (nonius::chronometer meter) {
            auto n = meter.param<N>();
            auto v = maker(n);
            measure(
                meter, [&] {
                    auto r = rrb_to_transient(v);
                    for (auto i = 0u; i < n; ++i)
                        r = transient_rrb_update(
                            r, i, reinterpret_cast<void*>(i));
                    return r;
                });
        };
}

template <typename Fn>
auto benchmark_assoc_mut_random_librrb(Fn maker)
{
    return
        [=] (nonius::chronometer meter) {
            auto n = meter.param<N>();
            auto v = maker(n);
            auto g = make_generator(n);
            measure(
                meter, [&] {
                    auto r = rrb_to_transient(v);
                    for (auto i = 0u; i < n; ++i)
                        r = transient_rrb_update(
                            r, g[i], reinterpret_cast<void*>(i));
                    return r;
                });
        };
}

} // anonymous namespace
