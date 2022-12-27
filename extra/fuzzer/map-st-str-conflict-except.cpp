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
#include <immer/map.hpp>

#include <immer/algorithm.hpp>

#include <array>
#include <boost/optional.hpp>

using st_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                       immer::unsafe_refcount_policy,
                                       immer::no_lock_policy,
                                       immer::no_transience_policy,
                                       false>;

struct colliding_hash_t
{
    std::size_t operator()(const std::string& x) const
    {
        return std::hash<std::string>{}(x) & ~((std::size_t{1} << 48) - 1);
    }
};

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
    constexpr auto var_count = 4;

    using map_t    = typename dadaist_wrapper<immer::map<std::string,
                                                      immer::box<std::string>,
                                                      colliding_hash_t,
                                                      std::equal_to<>,
                                                      st_memory,
                                                      3>>::type;
    using mapped_t = typename map_t::mapped_type;

    auto vars = std::array<map_t, var_count>{};

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    auto d = dadaism{};

    return fuzzer_input{data, size}.run(dada_step([&](auto& in) {
        enum ops
        {
            op_set,
            op_erase,
            op_set_move,
            op_erase_move,
            op_iterate,
            op_find,
            op_update,
            op_update_move,
            op_diff
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);

        auto step =
            read<char>(in) % 2 ? boost::make_optional(d.next()) : boost::none;

        switch (read<char>(in)) {
        case op_set: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = vars[src].set(value, {"foo"});
            break;
        }
        case op_erase: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = vars[src].erase(value);
            break;
        }
        case op_set_move: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = std::move(vars[src]).set(value, mapped_t{"foo"});
            break;
        }
        case op_erase_move: {
            auto value = std::to_string(read<size_t>(in));
            vars[dst]  = std::move(vars[src]).erase(value);
            break;
        }
        case op_iterate: {
            auto srcv = vars[src];
            for (const auto& v : srcv) {
                vars[dst] = vars[dst].set(v.first, v.second);
            }
            break;
        }
        case op_find: {
            auto value = std::to_string(read<size_t>(in));
            auto res   = vars[src].find(value);
            if (res != nullptr) {
                vars[dst] = vars[dst].set(res->value, mapped_t{"foo"});
            }
            break;
        }
        case op_update: {
            auto key  = std::to_string(read<size_t>(in));
            vars[dst] = vars[src].update(key, [](auto x) {
                return mapped_t{"baz"};
                // return mapped_t{*std::move(x).value + "bar"};
            });
            break;
        }
        case op_update_move: {
            auto key  = std::to_string(read<size_t>(in));
            vars[dst] = std::move(vars[src]).update(key, [](auto x) {
                return mapped_t{"baz"};
                // return mapped_t{*std::move(x).value + "baz"};
            });
            break;
        }
        case op_diff: {
            auto&& a = vars[src];
            auto&& b = vars[dst];
            diff(
                a,
                b,
                [&](auto&& x) {
                    // assert(!a.count(x.first));
                    // assert(b.count(x.first));
                },
                [&](auto&& x) {
                    // assert(a.count(x.first));
                    // assert(!b.count(x.first));
                },
                [&](auto&& x, auto&& y) {
                    // assert(x.first == y.first);
                    // assert(x.second != y.second);
                });
        }
        default:
            break;
        };
        return true;
    }));
}
