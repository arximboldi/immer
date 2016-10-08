
#pragma once

#include <utility>
#include <numeric>

#include <immer/config.hpp>
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

template <typename NodeT,  typename FnT>
auto update_visitor(FnT&& fn)
{
    using node_t = NodeT;
    return make_visitor(
        []  (auto&& pos, auto&& v, std::size_t idx) -> NodeT* {
            auto offset  = pos.index(idx);
            auto count   = pos.count();
            auto node    = node_t::copy_inner_r(pos.node(), count);
            node->inner()[offset]->dec_unsafe();
            node->inner()[offset] = pos.towards(v, idx, offset);
            return node;
        },
        []  (auto&& pos, auto&& v, std::size_t idx) -> NodeT*  {
            auto offset  = pos.index(idx);
            auto count   = pos.count();
            auto node    = node_t::copy_inner(pos.node(), count);
            node->inner()[offset]->dec_unsafe();
            node->inner()[offset] = pos.towards(v, idx, offset, count);
            return node;
        },
        [&]  (auto&& pos, auto&& v, std::size_t idx) -> NodeT*  {
            auto offset  = pos.index(idx);
            auto node    = node_t::copy_leaf(pos.node(), pos.count());
            node->leaf()[offset] = std::forward<FnT>(fn) (
                std::move(node->leaf()[offset]));
            return node;
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
