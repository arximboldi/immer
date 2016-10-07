
#pragma once

#include <utility>
#include <numeric>

namespace immer {
namespace detail {

template <typename Step, typename State, typename Fn>
State do_reduce(Step step, State init, Fn&& fn)
{
    std::forward<Fn>(fn) (
        [] (auto&& pos, auto&& ...recur) {
            pos.each(recur...);
        },
        [&] (auto&& pos, auto&&...) {
            auto data  = pos.node()->leaf();
            auto count = pos.count();
            init = std::accumulate(data,
                                   data + count,
                                   init,
                                   step);
        });
    return init;
}

template <typename Fn>
auto do_dec(Fn&& fn)
{
    return std::forward<Fn>(fn) (
        [] (auto&& pos, auto&& ...recur) {
            using node_t = std::decay_t<decltype(*pos.node())>;
            auto node = pos.node();
            if (node->dec()) {
                pos.each(recur...);
                node_t::delete_inner_r(node);
            }
        },
        [] (auto&& pos, auto&& ...recur) {
            using node_t = std::decay_t<decltype(*pos.node())>;
            auto node = pos.node();
            if (node->dec()) {
                pos.each(recur...);
                node_t::delete_inner(node);
            }
        },
        [] (auto&& pos, auto&&...) {
            using node_t = std::decay_t<decltype(*pos.node())>;
            auto node = pos.node();
            if (node->dec()) {
                auto count = pos.count();
                node_t::delete_leaf(node, count);
            }
        });
}

} // namespace detail
} // namespace immer
