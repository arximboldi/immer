//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"

#include <immer/sorted_set.hpp>
#include <immer/sorted_set_transient.hpp>

#include <array>
#include <cassert>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    constexpr auto var_count = 8;

    using set_t = immer::
        sorted_set<int, std::less<int>, immer::default_memory_policy, 2u, 2u>;

    auto vars = std::array<set_t, var_count>{};

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_insert,
            op_erase,
            op_insert_move,
            op_erase_move,
            op_transient_insert,
            op_transient_erase,
            op_iterate,
            op_find,
            op_lower_bound,
            op_equals,
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        auto key = static_cast<int>(read<std::uint8_t>(in));
        switch (read<char>(in)) {
        case op_insert:
            vars[dst] = vars[src].insert(key);
            break;
        case op_erase:
            vars[dst] = vars[src].erase(key);
            break;
        case op_insert_move:
            vars[dst] = std::move(vars[src]).insert(key);
            break;
        case op_erase_move:
            vars[dst] = std::move(vars[src]).erase(key);
            break;
        case op_transient_insert: {
            auto t = vars[src].transient();
            t.insert(key);
            t.insert(key + 1);
            vars[dst] = t.persistent();
            break;
        }
        case op_transient_erase: {
            auto t = vars[src].transient();
            t.erase(key);
            t.erase(key + 2);
            vars[dst] = t.persistent();
            break;
        }
        case op_iterate: {
            auto last = -1;
            auto n    = std::size_t{};
            for (auto&& x : vars[src]) {
                assert(x > last);
                last = x;
                ++n;
            }
            assert(n == vars[src].size());
            (void) last;
            (void) n;
            break;
        }
        case op_find: {
            auto p = vars[src].find(key);
            assert(vars[src].count(key) == (p ? 1u : 0u));
            (void) p;
            break;
        }
        case op_lower_bound: {
            auto it = vars[src].lower_bound(key);
            assert(it == vars[src].end() || *it >= key);
            (void) it;
            break;
        }
        case op_equals:
            if (vars[src] == vars[dst])
                assert(vars[src].size() == vars[dst].size());
            break;
        default:
            break;
        };
        assert(vars[dst].impl().check_tree());
        return true;
    });
}
