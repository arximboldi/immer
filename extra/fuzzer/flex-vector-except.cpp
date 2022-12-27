//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"

#include "../../test/dada.hpp"

#include <immer/box.hpp>
#include <immer/flex_vector.hpp>

#include <array>
#include <boost/optional.hpp>

template <typename Fn>
auto dada_step(Fn&& fn)
{
    return [fn](auto&& in) {
        try {
            return fn(in);
        } catch (const dada_error&) {
            return true;
        }
    };
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    constexpr auto var_count = 8;
    constexpr auto bits      = 2;

    using vector_t = typename dadaist_wrapper<
        immer::flex_vector<immer::box<int>,
                           immer::default_memory_policy,
                           bits,
                           bits>>::type;

    using value_t = typename vector_t::value_type;

    using size_t = std::uint8_t;

    auto vars = std::array<vector_t, var_count>{};

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };
    auto is_valid_var_neq = [](auto other) {
        return [=](auto idx) {
            return idx >= 0 && idx < var_count && idx != other;
        };
    };
    auto is_valid_index = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx < v.size(); };
    };
    auto is_valid_size = [](auto& v) {
        return [&](auto idx) { return idx >= 0 && idx <= v.size(); };
    };
    auto can_concat = [](auto&& v1, auto&& v2) {
        return v1.size() + v2.size() < vector_t::max_size();
    };
    auto can_compare = [](auto&& v) {
        // avoid comparing vectors that are too big, and hence, slow to compare
        return v.size() < (1 << 15);
    };

    auto d = dadaism{};

    return fuzzer_input{data, size}.run(dada_step([&](auto& in) {
        enum ops
        {
            op_push_back,
            op_update,
            op_take,
            op_drop,
            op_concat,
            op_push_back_move,
            op_update_move,
            op_take_move,
            op_drop_move,
            op_concat_move_l,
            op_concat_move_r,
            op_concat_move_lr,
            op_insert,
            op_erase,
            op_compare,
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);

        auto step =
            read<char>(in) % 2 ? boost::make_optional(d.next()) : boost::none;

        switch (read<char>(in)) {
        case op_push_back: {
            vars[dst] = vars[src].push_back(value_t{42});
            break;
        }
        case op_update: {
            auto idx  = read<size_t>(in, is_valid_index(vars[src]));
            vars[dst] = vars[src].update(idx, [](auto x) {
                return value_t{45};
                // value_t{*x.value + 1};
            });
            break;
        }
        case op_take: {
            auto idx  = read<size_t>(in, is_valid_size(vars[src]));
            vars[dst] = vars[src].take(idx);
            break;
        }
        case op_drop: {
            auto idx  = read<size_t>(in, is_valid_size(vars[src]));
            vars[dst] = vars[src].drop(idx);
            break;
        }
        case op_concat: {
            auto src2 = read<char>(in, is_valid_var);
            if (can_concat(vars[src], vars[src2]))
                vars[dst] = vars[src] + vars[src2];
            break;
        }
        case op_push_back_move: {
            vars[dst] = std::move(vars[src]).push_back(value_t{21});
            break;
        }
        case op_update_move: {
            auto idx  = read<size_t>(in, is_valid_index(vars[src]));
            vars[dst] = std::move(vars[src]).update(idx, [](auto x) {
                return value_t{82};
                // return value_t{*x.value + 1};
            });
            break;
        }
        case op_take_move: {
            auto idx  = read<size_t>(in, is_valid_size(vars[src]));
            vars[dst] = std::move(vars[src]).take(idx);
            break;
        }
        case op_drop_move: {
            auto idx  = read<size_t>(in, is_valid_size(vars[src]));
            vars[dst] = std::move(vars[src]).drop(idx);
            break;
        }
        case op_concat_move_l: {
            auto src2 = read<char>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2]))
                vars[dst] = std::move(vars[src]) + vars[src2];
            break;
        }
        case op_concat_move_r: {
            auto src2 = read<char>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2]))
                vars[dst] = vars[src] + std::move(vars[src2]);
            break;
        }
        case op_concat_move_lr: {
            auto src2 = read<char>(in, is_valid_var_neq(src));
            if (can_concat(vars[src], vars[src2]))
                vars[dst] = std::move(vars[src]) + std::move(vars[src2]);
            break;
        }
        case op_compare: {
            using std::swap;
            if (can_compare(vars[src]) && vars[src] == vars[dst])
                swap(vars[src], vars[dst]);
            break;
        }
        case op_erase: {
            auto idx  = read<size_t>(in, is_valid_index(vars[src]));
            vars[dst] = vars[src].erase(idx);
            break;
        }
        case op_insert: {
            auto idx  = read<size_t>(in, is_valid_size(vars[src]));
            vars[dst] = vars[src].insert(idx, value_t{42});
            break;
        }
        default:
            break;
        };
        return true;
    }));
}
