
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
struct array_for_visitor
{
    using this_t = array_for_visitor;

    template <typename PosT>
    friend T* visit_inner(this_t, PosT&& pos, std::size_t idx)
    { return pos.descend(this_t{}, idx); }

    template <typename PosT>
    friend T* visit_leaf(this_t, PosT&& pos, std::size_t)
    { return pos.node()->leaf(); }
};

template <typename T>
struct relaxed_array_for_visitor
{
    using this_t = relaxed_array_for_visitor;
    using result_t = std::tuple<T*, std::size_t, std::size_t>;

    template <typename PosT>
    friend result_t visit_inner(this_t, PosT&& pos, std::size_t idx)
    { return pos.towards(this_t{}, idx); }

    template <typename PosT>
    friend result_t visit_leaf(this_t, PosT&& pos, std::size_t idx)
    { return { pos.node()->leaf(), pos.index(idx), pos.count() }; }
};

template <typename T>
struct get_visitor
{
    using this_t = get_visitor;

    template <typename PosT>
    friend const T& visit_inner(this_t, PosT&& pos, std::size_t idx)
    { return pos.descend(this_t{}, idx); }

    template <typename PosT>
    friend const T& visit_leaf(this_t, PosT&& pos, std::size_t idx)
    { return pos.node()->leaf() [pos.index(idx)]; }
};

struct reduce_visitor
{
    using this_t = reduce_visitor;

    template <typename Pos, typename Step, typename State>
    friend void visit_inner(this_t, Pos&& pos, Step&& step, State& acc)
    { pos.each(this_t{}, step, acc); }

    template <typename Pos, typename Step, typename State>
    friend void visit_leaf(this_t, Pos&& pos, Step&& step, State& acc)
    {
        auto data  = pos.node()->leaf();
        auto count = pos.count();
        acc = std::accumulate(data,
                              data + count,
                              acc,
                              step);
    }
};

template <typename NodeT>
struct update_visitor
{
    using node_t = NodeT;
    using this_t = update_visitor;

    template <typename Pos, typename Fn>
    friend node_t* visit_relaxed(this_t, Pos&& pos, std::size_t idx, Fn&& fn)
    {
        auto offset  = pos.index(idx);
        auto count   = pos.count();
        auto next    = pos.towards_oh(this_t{}, idx, offset, fn);
        auto node    = node_t::copy_inner_r(pos.node(), count);
        node->inner()[offset]->dec_unsafe();
        node->inner()[offset] = next;
        return node->freeze();
    }

    template <typename Pos, typename Fn>
    friend node_t* visit_regular(this_t, Pos&& pos, std::size_t idx, Fn&& fn)
    {
        auto offset  = pos.index(idx);
        auto count   = pos.count();
        auto next    = pos.towards_oh_ch(this_t{}, idx, offset, count, fn);
        auto node    = node_t::copy_inner(pos.node(), count);
        node->inner()[offset]->dec_unsafe();
        node->inner()[offset] = next;
        return node->freeze();
    }

    template <typename Pos, typename Fn>
    friend node_t* visit_leaf(this_t, Pos&& pos, std::size_t idx, Fn&& fn)
    {
        auto offset  = pos.index(idx);
        auto node    = node_t::copy_leaf(pos.node(), pos.count());
        node->leaf()[offset] = std::forward<Fn>(fn) (
            std::move(node->leaf()[offset]));
        return node->freeze();
    };
};

struct dec_visitor
{
    using this_t = dec_visitor;

    template <typename Pos>
    friend void visit_relaxed(this_t, Pos&& p)
    {
        using node_t = node_type<Pos>;
        auto node = p.node();
        if (node->dec()) {
            p.each(this_t{});
            node_t::delete_inner_r(node);
        }
    }

    template <typename Pos>
    friend void visit_regular(this_t, Pos&& p)
    {
        using node_t = node_type<Pos>;
        auto node = p.node();
        if (node->dec()) {
            p.each(this_t{});
            node_t::delete_inner(node);
        }
    }

    template <typename Pos>
    friend void visit_leaf(this_t, Pos&& p)
    {
        using node_t = node_type<Pos>;
        auto node = p.node();
        if (node->dec()) {
            auto count = p.count();
            node_t::delete_leaf(node, count);
        }
    }
};

template <typename NodeT>
struct push_tail_visitor
{
    static constexpr auto B = NodeT::bits;

    using this_t = push_tail_visitor;
    using node_t = NodeT;

    template <typename Pos>
    friend node_t* visit_relaxed(this_t, Pos&& pos, node_t* tail, unsigned ts)
    {
        auto level       = pos.shift();
        auto idx         = pos.count() - 1;
        auto children    = pos.size(idx);
        auto new_idx     = children == 1 << level || level == B
            ? idx + 1 : idx;
        auto new_child   = (node_t*){};
        if (new_idx >= branches<B>)
            return nullptr;
        else if (idx == new_idx) {
            new_child = pos.last_oh_csh(this_t{}, idx, children, tail, ts);
            if (!new_child) {
                if (++new_idx < branches<B>)
                    new_child = node_t::make_path(level - B, tail);
                else
                    return nullptr;
            }
        } else
            new_child = node_t::make_path(level - B, tail);
        auto count       = new_idx + 1;
        auto new_parent  = node_t::copy_inner_r_n(count, pos.node(), new_idx);
        auto new_relaxed = new_parent->relaxed();
        new_parent->inner()[new_idx] = new_child;
        new_relaxed->sizes[new_idx] = pos.size() + ts;
        new_relaxed->count = count;
        return new_parent->freeze();
    }

    template <typename Pos, typename... Args>
    friend node_t* visit_regular(this_t, Pos&& pos, node_t* tail, Args&&...)
    {
        assert((pos.size() & mask<B>) == 0);
        auto idx         = pos.index(pos.size() - 1);
        auto new_idx     = pos.index(pos.size() + branches<B> - 1);
        auto count       = new_idx + 1;
        auto new_child   = idx == new_idx
            ? pos.last_oh(this_t{}, idx, tail)
            : node_t::make_path(pos.shift() - B, tail);
        auto new_parent  = node_t::copy_inner_n(count, pos.node(), new_idx);
        new_parent->inner()[new_idx] = new_child;
        return new_parent->freeze();
    }

    template <typename Pos, typename... Args>
    friend node_t* visit_leaf(this_t, Pos&& pos, node_t* tail, Args&&...)
    { return tail; };
};

template <typename NodeT, bool Collapse=true>
struct slice_right_visitor
{
    using node_t = NodeT;
    using this_t = slice_right_visitor;

    // returns a new shift, new root, the new tail size and the new tail
    using result_t = std::tuple<unsigned, NodeT*, unsigned, NodeT*>;
    using no_collapse_t = slice_right_visitor<NodeT, false>;

    template <typename PosT>
    friend result_t visit_relaxed(this_t, PosT&& pos, std::size_t last)
    {
        constexpr auto B = NodeT::bits;
        auto idx = pos.index(last);
        if (Collapse && idx == 0) {
            return pos.towards_oh(this_t{}, last, idx);
        } else {
            using std::get;
            auto subs = pos.towards_oh(no_collapse_t{}, last, idx);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            if (next) {
                auto count = idx + 1;
                auto newn  = node_t::copy_inner_r_n(count, pos.node(), idx);
                auto newr  = newn->relaxed();
                newn->inner()[idx] = next;
                newr->sizes[idx] = last + 1 - ts;
                newr->count = count;
                return { pos.shift(), newn->freeze(), ts, tail };
            } else if (idx == 0) {
                return { pos.shift(), nullptr, ts, tail };
            } else if (Collapse && idx == 1 && pos.shift() > B) {
                auto newn = pos.node()->inner()[0];
                return { pos.shift() - B, newn->inc(), ts, tail };
            } else {
                auto newn = node_t::copy_inner_r(pos.node(), idx);
                return { pos.shift(), newn->freeze(), ts, tail };
            }
        }
    }

    template <typename PosT>
    friend result_t visit_regular(this_t, PosT&& pos, std::size_t last)
    {
        constexpr auto B = NodeT::bits;
        auto idx = pos.index(last);
        if (Collapse && idx == 0) {
            return pos.towards_oh(this_t{}, last, idx);
        } else {
            using std::get;
            auto subs = pos.towards_oh(no_collapse_t{}, last, idx);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            if (next) {
                auto count = idx + 1;
                auto newn  = node_t::copy_inner_n(count, pos.node(), idx);
                newn->inner()[idx] = next;
                return { pos.shift(), newn->freeze(), ts, tail };
            } else if (idx == 0) {
                return { pos.shift(), nullptr, ts, tail };
            } else if (Collapse && idx == 1 && pos.shift() > B) {
                auto newn = pos.node()->inner()[0];
                return { pos.shift() - B, newn->inc(), ts, tail };
            } else {
                auto newn = node_t::copy_inner(pos.node(), idx);
                return { pos.shift(), newn->freeze(), ts, tail };
            }
        }
    }

    template <typename PosT>
    friend result_t visit_leaf(this_t, PosT&& pos, std::size_t last)
    {
        auto old_tail_size = pos.count();
        auto new_tail_size = pos.index(last) + 1;
        auto new_tail      = new_tail_size == old_tail_size
            ? pos.node()->inc()
            : node_t::copy_leaf(pos.node(), new_tail_size)->freeze();
        return { 0, nullptr, new_tail_size, new_tail };
    };
};

template <typename NodeT, bool Collapse=true>
struct slice_left_visitor
{
    using node_t = NodeT;
    using this_t = slice_left_visitor;

    // returns a new shift and new root
    using result_t = std::tuple<unsigned, NodeT*>;
    using no_collapse_t = slice_left_visitor<NodeT, false>;

    static constexpr auto B = NodeT::bits;

    template <typename PosT>
    friend result_t visit_inner(this_t, PosT&& pos, std::size_t first)
    {
        auto idx    = pos.subindex(first);
        auto count  = pos.count();
        auto left_size  = pos.size_before(idx);
        auto child_size = pos.size_sbh(idx, left_size);
        auto dropped_size = first;
        auto child_dropped_size = dropped_size - left_size;
        if (Collapse && pos.shift() > B && idx == pos.count() - 1) {
            return pos.towards_sub_oh(this_t{}, first, idx);
        } else {
            using std::get;
            auto n     = pos.node();
            auto subs  = pos.towards_sub_oh(no_collapse_t{}, first, idx);
            auto newn  = node_t::make_inner_r_n(count - idx);
            auto newr  = newn->relaxed();
            newr->count = count - idx;
            newr->sizes[0] = child_size - child_dropped_size;
            pos.copy_sizes(idx + 1, newr->count - 1,
                           newr->sizes[0], newr->sizes + 1);
            assert(newr->sizes[newr->count - 1] == pos.size() - dropped_size);
            newn->inner()[0] = get<1>(subs);
            std::uninitialized_copy(n->inner() + idx + 1,
                                    n->inner() + count,
                                    newn->inner() + 1);
            node_t::inc_nodes(newn->inner() + 1, newr->count - 1);
            return { pos.shift(), newn->freeze() };
        }
    }

    template <typename PosT>
    friend result_t visit_leaf(this_t, PosT&& pos, std::size_t first)
    {
        return {
            0,
            node_t::copy_leaf(pos.node(),
                              first & mask<B>,
                              pos.count())->freeze()
        };
    };
};

} // namespace rbts
} // namespace detail
} // namespace immer
