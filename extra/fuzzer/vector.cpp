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

#include "fuzzer_input.hpp"
#include <immer/vector.hpp>
#include <iostream>

extern "C"
int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    using vector_t = immer::vector<int, immer::default_memory_policy, 3, 3>;
    using size_t   = std::uint8_t;
    constexpr auto var_count = 4;

    auto vars = std::array<vector_t, var_count>{};

    auto is_valid_var = [&] (auto idx) {
        return idx >= 0 && idx < var_count;
    };
    auto is_valid_index = [] (auto& v) {
        return [&] (auto idx) { return idx >= 0 && idx < v.size(); };
    };
    auto is_valid_size = [] (auto& v) {
        return [&] (auto idx) { return idx >= 0 && idx <= v.size(); };
    };

    return fuzzer_input{data, size}.run([&] (auto& in)
    {
        enum ops {
            op_push_back,
            op_update,
            op_take,
            op_push_back_move,
            op_update_move,
            op_take_move,
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in))
        {
        case op_push_back:
            vars[dst] = vars[src]
                .push_back(42);
            break;
        case op_update:
            vars[dst] = vars[src]
                .update(read<size_t>(in, is_valid_index(vars[src])),
                        [] (auto x) { return x + 1; });
            break;
        case op_take:
            vars[dst] = vars[src]
                .take(read<std::size_t>(in, is_valid_size(vars[src])));
            break;
        case op_push_back_move: {
            vars[dst] = std::move(vars[src])
                .push_back(read<char>(in));
            break;
        }
        case op_update_move: {
            vars[dst] = std::move(vars[src])
                .update(read<size_t>(in, is_valid_index(vars[src])),
                        [] (auto x) { return x + 1; });
            break;
        }
        case op_take_move: {
            vars[dst] = std::move(vars[src])
                .take(read<size_t>(in, is_valid_size(vars[src])));
            break;
        }
        default:
            break;
        };
        return true;
    });
}
