//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"
#include <immer/set.hpp>
#include <iostream>
#include <array>

extern "C"
int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    constexpr auto var_count = 4;

    using set_t= immer::set<int>;
    // using size_t   = std::uint8_t;

    auto vars = std::array<set_t, var_count>{};

    auto is_valid_var = [&] (auto idx) {
        return idx >= 0 && idx < var_count;
    };
    /*
    auto is_valid_index = [] (auto& v) {
        return [&] (auto idx) { return idx >= 0 && idx < v.size(); };
    };
    auto is_valid_size = [] (auto& v) {
        return [&] (auto idx) { return idx >= 0 && idx <= v.size(); };
    };
    */

    return fuzzer_input{data, size}.run([&] (auto& in)
    {
        enum ops {
            op_insert,
            op_erase,
            op_insert_move,
            op_erase_move,
            iterate
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in))
        {
        case op_insert: {
            auto value = read<size_t>(in);
            vars[dst] = vars[src].insert(value);
            break;
        }
        case op_erase: {
            auto value = read<size_t>(in);
            vars[dst] = vars[src].erase(value);
            break;
        }
        case op_insert_move: {
            auto value = read<size_t>(in);
            vars[dst] = std::move(vars[src]).insert(value);
            break;
        }
        case op_erase_move: {
            auto value = read<size_t>(in);
            vars[dst] = vars[src].erase(value);
            break;
        }
        case iterate: {
            if(src != dst) {
                for(auto it = vars[src].begin(); it != vars[src].end(); ++it) {
                    vars[dst] = vars[dst].insert(*it);
                }
            }
            break;
        }
        default:
            break;
        };
        return true;
    });
}
