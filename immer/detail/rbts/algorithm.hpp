
#pragma once

#include <utility>
#include <numeric>
#include <memory>

#include <immer/config.hpp>
#include <immer/detail/rbts/position.hpp>
#include <immer/detail/rbts/visitor.hpp>

namespace immer {
namespace detail {
namespace rbts {

template <typename T>
auto array_for_visitor()
{
    return make_visitor(
        [] (auto&& v, auto&& pos, std::size_t idx) -> T* {
            return pos.descend(v, idx);
        },
        [] (auto&&, auto&& pos, std::size_t) -> T* {
            return pos.node()->leaf();
        });
}

template <typename T>
auto relaxed_array_for_visitor()
{
    using result_t = std::tuple<T*, std::size_t, std::size_t>;
    return make_visitor(
        [] (auto&& v, auto&& pos, std::size_t idx) -> result_t {
            return pos.towards(v, idx);
        },
        [] (auto&&, auto&& pos, std::size_t idx) -> result_t {
            return {
                pos.node()->leaf(),
                pos.subindex(idx),
                pos.count()
            };
        });
}

template <typename T>
auto get_visitor()
{
    return make_visitor(
        [] (auto&& v, auto&& pos, std::size_t idx) -> const T* {
            return pos.descend(v, idx);
        },
        [] (auto&&, auto&& pos, std::size_t idx) -> const T* {
            return &pos.node()->leaf() [pos.index(idx)];
        });
}

template <typename Step, typename State>
auto reduce_visitor(Step&& step, State& acc)
{
    return make_visitor(
        [] (auto&& v, auto&& pos) {
            pos.each(v);
        },
        [&] (auto&&, auto&& pos) {
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
        [] (auto&& v, auto&& pos, std::size_t idx) -> NodeT* {
            auto offset  = pos.subindex(idx);
            auto count   = pos.count();
            auto node    = node_t::copy_inner_r(pos.node(), count);
            node->inner()[offset]->dec_unsafe();
            node->inner()[offset] = pos.towards(v, idx, offset);
            return node;
        },
        [] (auto&& v, auto&& pos, std::size_t idx) -> NodeT*  {
            auto offset  = pos.subindex(idx);
            auto count   = pos.count();
            auto node    = node_t::copy_inner(pos.node(), count);
            node->inner()[offset]->dec_unsafe();
            node->inner()[offset] = pos.towards(v, idx, offset, count);
            return node;
        },
        [&] (auto&& v, auto&& pos, std::size_t idx) -> NodeT*  {
            auto offset  = pos.subindex(idx);
            auto node    = node_t::copy_leaf(pos.node(), pos.count());
            node->leaf()[offset] = std::forward<FnT>(fn) (
                std::move(node->leaf()[offset]));
            return node;
        });
}

auto dec_visitor()
{
    return make_visitor(
        [] (auto&& v, auto&& pos) {
            using node_t = std::decay_t<decltype(*pos.node())>;
            auto node = pos.node();
            if (node->dec()) {
                pos.each(v);
                node_t::delete_inner_r(node);
            }
        },
        [] (auto&& v, auto&& pos) {
            using node_t = std::decay_t<decltype(*pos.node())>;
            auto node = pos.node();
            if (node->dec()) {
                pos.each(v);
                node_t::delete_inner(node);
            }
        },
        [] (auto&&, auto&& pos) {
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
        [=] (auto&& v, auto&& pos) -> NodeT* {
            constexpr auto B = NodeT::bits;
            auto level       = pos.shift();
            auto idx         = pos.count() - 1;
            auto children    = pos.size(idx);
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
        [=] (auto&& v, auto&& pos) -> NodeT* {
            constexpr auto B = NodeT::bits;
            auto idx         = pos.subindex(pos.size() - 1);
            auto new_idx     = pos.subindex(pos.size() + ts - 1);
            auto new_parent  = node_t::copy_inner(pos.node(), new_idx);
            new_parent->inner()[new_idx] =
                idx == new_idx   ? pos.last(v, idx)
                /* otherwise */  : node_t::make_path(pos.shift() - B, tail);
            return new_parent;
        },
        [=] (auto&&, auto&& pos) -> NodeT* {
            return tail;
        });
}

template <typename NodeT, bool Collapse>
struct slice_right_visitor_t
{
    // returns a new shift, new root, the new tail size and the new tail
    using result_t = std::tuple<unsigned, NodeT*, unsigned, NodeT*>;
    using node_t = NodeT;

    template <typename PosT>
    friend result_t visit_relaxed(slice_right_visitor_t v,
                                  PosT&& pos,
                                  std::size_t last)
    {
        constexpr auto B = NodeT::bits;
        auto idx = pos.subindex(last);
        if (Collapse && idx == 0) {
            return pos.towards(v, last, idx);
        } else {
            using std::get;
            auto fv   = slice_right_visitor_t<NodeT, false>{};
            auto subs = pos.towards(fv, last, idx);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            if (next) {
                auto newn = node_t::copy_inner_r(pos.node(), idx);
                auto newr = newn->relaxed();
                newn->inner()[idx] = next;
                newr->sizes[idx] = last + 1 - ts;
                newr->count++;
                return { pos.shift(), newn, ts, tail };
            } else if (idx == 0) {
                return { pos.shift(), nullptr, ts, tail };
            } else if (Collapse && idx == 1 && pos.shift() > B) {
                auto newn = pos.node()->inner()[0];
                return { pos.shift() - B, newn->inc(), ts, tail };
            } else {
                auto newn = node_t::copy_inner_r(pos.node(), idx);
                return { pos.shift(), newn, ts, tail };
            }
        }
    }

    template <typename PosT>
    friend result_t visit_regular(slice_right_visitor_t v,
                                  PosT&& pos,
                                  std::size_t last)
    {
        constexpr auto B = NodeT::bits;
        auto idx = pos.subindex(last);
        if (Collapse && idx == 0) {
            return pos.towards(v, last, idx);
        } else {
            using std::get;
            auto fv   = slice_right_visitor_t<NodeT, false>();
            auto subs = pos.towards(fv, last, idx);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            if (next) {
                auto newn = node_t::copy_inner(pos.node(), idx);
                newn->inner()[idx] = next;
                return { pos.shift(), newn, ts, tail };
            } else if (idx == 0) {
                return { pos.shift(), nullptr, ts, tail };
            } else if (Collapse && idx == 1 && pos.shift() > B) {
                auto newn = pos.node()->inner()[0];
                return { pos.shift() - B, newn->inc(), ts, tail };
            } else {
                auto newn = node_t::copy_inner(pos.node(), idx);
                return { pos.shift(), newn, ts, tail };
            }
        }
    }

    template <typename PosT>
    friend result_t visit_leaf(slice_right_visitor_t v,
                               PosT&& pos,
                               std::size_t last)
    {
        auto old_tail_size = pos.count();
        auto new_tail_size = pos.subindex(last) + 1;
        auto new_tail      = new_tail_size == old_tail_size
            ? pos.node()->inc()
            : node_t::copy_leaf(pos.node(), new_tail_size);
        return { 0, nullptr, new_tail_size, new_tail };
    };
};

template <typename NodeT>
auto slice_right_visitor()
{
    return slice_right_visitor_t<NodeT, true>{};
}

template <typename NodeT, bool Collapse>
struct slice_left_visitor_t
{
    // returns a new shift and new root
    using result_t = std::tuple<unsigned, NodeT*>;
    using node_t = NodeT;

    static constexpr auto B = NodeT::bits;

    template <typename PosT>
    friend result_t visit_inner(slice_left_visitor_t v,
                                PosT&& pos,
                                std::size_t first)
    {
        auto idx    = pos.subindex(first);
        auto count  = pos.count();
        auto this_size  = pos.size();
        auto left_size  = pos.size_before(idx);
        auto child_size = pos.size(idx, left_size);
        auto dropped_size = first;
        auto child_dropped_size = dropped_size - left_size;
        if (Collapse && pos.shift() > B && idx == pos.count() - 1) {
            return pos.towards(v, first, idx);
        } else {
            using std::get;
            auto n     = pos.node();
            auto fv    = slice_left_visitor_t<NodeT, false>{};
            auto subs  = pos.towards(fv, first, idx);
            auto newn  = node_t::make_inner_r();
            auto newr  = newn->relaxed();
            newr->count = count - idx;
            newr->sizes[0] = child_size - child_dropped_size;
            pos.copy_sizes(idx + 1, newr->count - 1,
                           newr->sizes[0], newr->sizes + 1);
            assert(newr->sizes[newr->count - 1] == this_size - dropped_size);
            newn->inner()[0] = get<1>(subs);
            std::uninitialized_copy(n->inner() + idx + 1,
                                    n->inner() + count,
                                    newn->inner() + 1);
            node_t::inc_nodes(newn->inner() + 1, newr->count - 1);
            return { pos.shift(), newn };
        }
    }

    template <typename PosT>
    friend result_t visit_leaf(slice_left_visitor_t,
                               PosT&& pos,
                               std::size_t first)
    {
        return {
            0,
            node_t::copy_leaf(pos.node(),
                              first & mask<B>,
                              pos.count())
        };
    };
};

template <typename NodeT>
auto slice_left_visitor()
{
    return slice_left_visitor_t<NodeT, true>{};
}

} // namespace rbts
} // namespace detail
} // namespace immer
