//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "fuzzer_input.hpp"

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/sorted_map.hpp>
#include <immer/sorted_map_transient.hpp>

#include <array>
#include <cassert>

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    constexpr auto var_count = 8;

    using map_t = immer::sorted_map<int, int, std::less<int>, gc_memory, 2u, 2u>;

    auto vars = std::array<map_t, var_count>{};

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_set,
            op_erase,
            op_update,
            op_transient_set,
            op_transient_erase,
            op_iterate,
            op_find,
            op_lower_bound,
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        auto key = static_cast<int>(read<std::uint8_t>(in));
        switch (read<char>(in)) {
        case op_set:
            vars[dst] = vars[src].set(key, key);
            break;
        case op_erase:
            vars[dst] = vars[src].erase(key);
            break;
        case op_update:
            vars[dst] = vars[src].update(key, [](int x) { return x + 1; });
            break;
        case op_transient_set: {
            auto t = vars[src].transient();
            t.set(key, key);
            t.set(key + 1, key);
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
            for (auto&& kv : vars[src]) {
                assert(kv.first > last);
                last = kv.first;
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
            assert(it == vars[src].end() || it->first >= key);
            (void) it;
            break;
        }
        default:
            break;
        };
        assert(vars[dst].impl().check_tree());
        return true;
    });
}
