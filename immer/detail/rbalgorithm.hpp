
#pragma once

#include <utility>
#include <numeric>

#include <immer/detail/rbpos.hpp>

namespace immer {
namespace detail {

template <typename Step, typename State>
auto reduce_visitor(Step&& step, State& acc)
{
    return make_visitor(
        [] (auto&& pos, auto&& v) {
            pos.each(v);
        },
        [&] (auto&& pos, auto&&) {
            auto data  = pos.node()->leaf();
            auto count = pos.count();
            acc = std::accumulate(data,
                                  data + count,
                                  acc,
                                  step);
        });
}

auto dec_visitor()
{
    return make_visitor(
        [] (auto&& pos, auto&& v) {
            using node_t = std::decay_t<decltype(*pos.node())>;
            auto node = pos.node();
            if (node->dec()) {
                pos.each(v);
                node_t::delete_inner_r(node);
            }
        },
        [] (auto&& pos, auto&& v) {
            using node_t = std::decay_t<decltype(*pos.node())>;
            auto node = pos.node();
            if (node->dec()) {
                pos.each(v);
                node_t::delete_inner(node);
            }
        },
        [] (auto&& pos, auto&&) {
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
