//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"
#include <immer/map.hpp>
#include <iostream>
#include <array>

extern "C"
int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    constexpr auto var_count = 4;

    using map_t = immer::map<char, int>;
    // using size_t   = std::uint8_t;

    auto vars = std::array<map_t, var_count>{};

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
            op_set,
            op_erase,
            op_set_move,
            op_erase_move,
            iterate,
            find,
            update
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in))
        {
        case op_set: {
            auto value = read<size_t>(in);
            vars[dst] = vars[src].set(value, 42);
            break;
        }
        case op_erase: {
            auto value = read<size_t>(in);
            vars[dst] = vars[src].erase(value);
            break;
        }
        case op_set_move: {
            auto value = read<size_t>(in);
            vars[dst] = std::move(vars[src]).set(value, 42);
            break;
        }
        case op_erase_move: {
            auto value = read<size_t>(in);
            vars[dst] = std::move(vars[src]).erase(value);
            break;
        }
        case iterate: {
            if(src != dst) {
                for(auto it = vars[src].begin(); it != vars[src].end(); ++it) {
                    vars[dst] = vars[dst].set(it->first, it->second);
                }
            }
            break;
        }
        case find: {
            auto value = read<size_t>(in);
            auto res = vars[src].find(value);
            if(res != nullptr) {
                vars[dst] = vars[dst].set(*res, 42);
            }
            break;
        }
        case update: {
            auto key = read<size_t>(in);
            vars[dst] = vars[src].update(key, [](int x) { return x+1; });
            break;
        }
        default:
            break;
        };
        return true;
    });
}
