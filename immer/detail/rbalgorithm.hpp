
#pragma once

#include <utility>
#include <numeric>

#include <immer/config.hpp>
#include <immer/detail/rbpos.hpp>

namespace immer {
namespace detail {

template <typename T>
auto array_for_visitor()
{
    return make_visitor(
        [] (auto&& pos, auto&& v, std::size_t idx) -> T* {
            return pos.descend(v, idx);
        },
        [] (auto&& pos, auto&&, std::size_t) -> T* {
            return pos.node()->leaf();
        });
}

template <typename T>
auto get_visitor()
{
    return make_visitor(
        [] (auto&& pos, auto&& v, std::size_t idx) -> const T* {
            return pos.descend(v, idx);
        },
        [] (auto&& pos, auto&&, std::size_t idx) -> const T* {
            return &pos.node()->leaf() [pos.index(idx)];
        });
}

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

template <typename NodeT>
auto push_tail_visitor(NodeT* tail, unsigned ts)
{
    using node_t = NodeT;

    return make_visitor(
        [=] (auto&& pos, auto&& v) -> NodeT* {
            constexpr auto B = NodeT::bits;
            auto level       = pos.shift();
            auto idx         = pos.count() - 1;
            auto children    = pos.child_size(idx);
            auto new_idx     = children == 1 << level || level == B
                ? idx + 1 : idx;
            auto new_child   = (node_t*){};
            if (new_idx >= branches<B>)
                return nullptr;
            else if (idx == new_idx) {
                new_child = pos.last(v, idx, children);
                if (!new_child) {
                    if (++new_idx < branches<B>)
                        new_child = node_t::make_path(level - B, tail);
                    else
                        return nullptr;
                }
            } else
                new_child = node_t::make_path(level - B, tail);
            auto new_parent  = node_t::copy_inner_r(pos.node(), new_idx);
            auto new_relaxed = new_parent->relaxed();
            new_parent->inner()[new_idx] = new_child;
            new_relaxed->sizes[new_idx] = pos.size() + ts;
            new_relaxed->count = new_idx + 1;
            return new_parent;
        },
        [=] (auto&& pos, auto&& v) -> NodeT* {
            constexpr auto B = NodeT::bits;
            auto idx         = pos.index(pos.size() - 1);
            auto new_idx     = pos.index(pos.size() + ts - 1);
            auto new_parent  = node_t::copy_inner(pos.node(), new_idx);
            new_parent->inner()[new_idx] =
                idx == new_idx   ? pos.last(v, idx)
                /* otherwise */  : node_t::make_path(pos.shift() - B, tail);
            return new_parent;
        },
        [=] (auto&& pos, auto&&...) -> NodeT* {
            return tail;
        });
}

} // namespace detail
} // namespace immer
