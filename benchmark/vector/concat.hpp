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

#include "benchmark/vector/common.hpp"

namespace {

constexpr auto concat_steps = 10u;

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
}

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
            auto n = meter.param<N>();

            auto v = Vektor{};
            for (auto i = 0u; i < n / concat_steps; ++i)
                v = PushFn{}(std::move(v), i);

            measure(meter, [&] {
                    auto r = Vektor{};
                    for (auto i = 0u; i < concat_steps; ++i)
                        r = r + v;
                    return r;
                });
        };
}

template <typename Vektor>
auto benchmark_concat_incr_mut()
{
    return
        [] (nonius::chronometer meter)
        {
            auto n = meter.param<N>();

            auto v = Vektor{}.transient();
            for (auto i = 0u; i < n / concat_steps; ++i)
                v.push_back(i);

            measure(meter, [&] (int run) {
                auto r = Vektor{}.transient();
                for (auto i = 0u; i < concat_steps; ++i)
                    r.append(v);
                return r;
            });
        };
}

template <typename Vektor>
auto benchmark_concat_incr_mut2()
{
    return
        [] (nonius::chronometer meter)
        {
            auto n = meter.param<N>();

            using transient_t = typename Vektor::transient_type;
            using steps_t = std::vector<transient_t, gc_allocator<transient_t>>;
            auto vs = std::vector<steps_t, gc_allocator<steps_t>>(meter.runs());
            for (auto k = 0u; k < vs.size(); ++k) {
                vs[k].reserve(concat_steps);
                for (auto j = 0u; j < concat_steps; ++j) {
                    auto vv = Vektor{}.transient();
                    for (auto i = 0u; i < n / concat_steps; ++i)
                        vv.push_back(i);
                    vs[k].push_back(std::move(vv));
                }
            }
            measure(meter, [&] (int run) {
                auto& vr = vs[run];
                auto r = Vektor{}.transient();
                assert(vr.size() == concat_steps);
                for (auto i = 0u; i < concat_steps; ++i)
                    r.append(std::move(vr[i]));
                return r;
            });
        };
}

template <typename Vektor>
auto benchmark_concat_incr_chunkedseq()
{
    return
        [] (nonius::chronometer meter)
        {
            auto n = meter.param<N>();

            using steps_t = std::vector<Vektor>;
            auto vs = std::vector<steps_t>(meter.runs());
            for (auto k = 0u; k < vs.size(); ++k) {
                for (auto j = 0u; j < concat_steps; ++j) {
                    auto vv = Vektor{};
                    for (auto i = 0u; i < n / concat_steps; ++i)
                        vv.push_back(i);
                    vs[k].push_back(std::move(vv));
                }
            }
            measure(meter, [&] (int run) {
                auto& vr = vs[run];
                auto r = Vektor{};
                for (auto i = 0u; i < concat_steps; ++i)
                    r.concat(vr[i]);
                return r;
            });
        };
}

template <typename Fn>
auto benchmark_concat_incr_librrb(Fn maker)
{
    return
        [=] (nonius::chronometer meter) {
            auto n = meter.param<N>();
            auto v = maker(n / concat_steps);
            measure(meter, [&] {
                    auto r = rrb_create();
                    for (auto i = 0ul; i < concat_steps; ++i)
                        r = rrb_concat(r, v);
                    return r;
                });
        };
}

} // anonymous namespace
