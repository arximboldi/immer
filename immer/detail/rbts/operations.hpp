//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <algorithm>
#include <memory>
#include <numeric>
#include <utility>

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
    friend T* visit_inner(this_t, PosT&& pos, size_t idx)
    { return pos.descend(this_t{}, idx); }

    template <typename PosT>
    friend T* visit_leaf(this_t, PosT&& pos, size_t)
    { return pos.node()->leaf(); }
};

template <typename T>
struct relaxed_array_for_visitor
{
    using this_t = relaxed_array_for_visitor;
    using result_t = std::tuple<T*, size_t, size_t>;

    template <typename PosT>
    friend result_t visit_inner(this_t, PosT&& pos, size_t idx)
    { return pos.towards(this_t{}, idx); }

    template <typename PosT>
    friend result_t visit_leaf(this_t, PosT&& pos, size_t idx)
    { return { pos.node()->leaf(), pos.index(idx), pos.count() }; }
};

template <typename T>
struct get_visitor
{
    using this_t = get_visitor;

    template <typename PosT>
    friend const T& visit_inner(this_t, PosT&& pos, size_t idx)
    { return pos.descend(this_t{}, idx); }

    template <typename PosT>
    friend const T& visit_leaf(this_t, PosT&& pos, size_t idx)
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
    friend node_t* visit_relaxed(this_t, Pos&& pos, size_t idx, Fn&& fn)
    {
        auto offset  = pos.index(idx);
        auto count   = pos.count();
        auto node    = node_t::copy_inner_r(pos.node(), count);
        node->inner()[offset]->dec_unsafe();
        node->inner()[offset] = pos.towards_oh(this_t{}, idx, offset, fn);
        return node;
    }

    template <typename Pos, typename Fn>
    friend node_t* visit_regular(this_t, Pos&& pos, size_t idx, Fn&& fn)
    {
        auto offset  = pos.index(idx);
        auto count   = pos.count();
        auto node    = node_t::copy_inner(pos.node(), count);
        node->inner()[offset]->dec_unsafe();
        node->inner()[offset] = pos.towards_oh_ch(this_t{}, idx, offset, count, fn);
        return node;
    }

    template <typename Pos, typename Fn>
    friend node_t* visit_leaf(this_t, Pos&& pos, size_t idx, Fn&& fn)
    {
        auto offset  = pos.index(idx);
        auto node    = node_t::copy_leaf(pos.node(), pos.count());
        node->leaf()[offset] = std::forward<Fn>(fn) (
            std::move(node->leaf()[offset]));
        return node;
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
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using this_t = push_tail_visitor;
    using node_t = NodeT;

    template <typename Pos>
    friend node_t* visit_relaxed(this_t, Pos&& pos, node_t* tail, count_t ts)
    {
        auto level       = pos.shift();
        auto idx         = pos.count() - 1;
        auto children    = pos.size(idx);
        auto new_idx     = children == size_t{1} << level || level == BL
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
        try {
            auto count       = new_idx + 1;
            auto new_parent  = node_t::copy_inner_r_n(count, pos.node(), new_idx);
            auto new_relaxed = new_parent->relaxed();
            new_parent->inner()[new_idx] = new_child;
            new_relaxed->sizes[new_idx] = pos.size() + ts;
            new_relaxed->count = count;
            return new_parent;
        } catch (...) {
            auto shift = pos.shift();
            auto size  = new_idx == idx ? children + ts : ts;
            if (shift > BL) {
                tail->inc();
                visit_maybe_relaxed_sub(new_child, shift - B, size, dec_visitor{});
            }
            throw;
        }
    }

    template <typename Pos, typename... Args>
    friend node_t* visit_regular(this_t, Pos&& pos, node_t* tail, Args&&...)
    {
        assert((pos.size() & mask<BL>) == 0);
        auto idx         = pos.index(pos.size() - 1);
        auto new_idx     = pos.index(pos.size() + branches<BL> - 1);
        auto count       = new_idx + 1;
        auto new_parent  = node_t::make_inner_n(count);
        try {
            new_parent->inner()[new_idx] =
                idx == new_idx  ? pos.last_oh(this_t{}, idx, tail)
                /* otherwise */ : node_t::make_path(pos.shift() - B, tail);
        } catch (...) {
            node_t::delete_inner(new_parent);
            throw;
        }
        return node_t::do_copy_inner(new_parent, pos.node(), new_idx);
    }

    template <typename Pos, typename... Args>
    friend node_t* visit_leaf(this_t, Pos&& pos, node_t* tail, Args&&...)
    { IMMER_UNREACHABLE; };
};

template <typename NodeT, bool Collapse=true>
struct slice_right_visitor
{
    using node_t = NodeT;
    using this_t = slice_right_visitor;

    // returns a new shift, new root, the new tail size and the new tail
    using result_t = std::tuple<shift_t, NodeT*, count_t, NodeT*>;
    using no_collapse_t = slice_right_visitor<NodeT, false>;

    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    template <typename PosT>
    friend result_t visit_relaxed(this_t, PosT&& pos, size_t last)
    {
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
                return { pos.shift(), newn, ts, tail };
            } else if (idx == 0) {
                return { pos.shift(), nullptr, ts, tail };
            } else if (Collapse && idx == 1 && pos.shift() > BL) {
                auto newn = pos.node()->inner()[0];
                return { pos.shift() - B, newn->inc(), ts, tail };
            } else {
                auto newn = node_t::copy_inner_r(pos.node(), idx);
                return { pos.shift(), newn, ts, tail };
            }
        }
    }

    template <typename PosT>
    friend result_t visit_regular(this_t, PosT&& pos, size_t last)
    {
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
                return { pos.shift(), newn, ts, tail };
            } else if (idx == 0) {
                return { pos.shift(), nullptr, ts, tail };
            } else if (Collapse && idx == 1 && pos.shift() > BL) {
                auto newn = pos.node()->inner()[0];
                return { pos.shift() - B, newn->inc(), ts, tail };
            } else {
                auto newn = node_t::copy_inner(pos.node(), idx);
                return { pos.shift(), newn, ts, tail };
            }
        }
    }

    template <typename PosT>
    friend result_t visit_leaf(this_t, PosT&& pos, size_t last)
    {
        auto old_tail_size = pos.count();
        auto new_tail_size = pos.index(last) + 1;
        auto new_tail      = new_tail_size == old_tail_size
            ? pos.node()->inc()
            : node_t::copy_leaf(pos.node(), new_tail_size);
        return { 0, nullptr, new_tail_size, new_tail };
    };
};

template <typename NodeT, bool Collapse=true>
struct slice_left_visitor
{
    using node_t = NodeT;
    using this_t = slice_left_visitor;

    // returns a new shift and new root
    using result_t = std::tuple<shift_t, NodeT*>;
    using no_collapse_t = slice_left_visitor<NodeT, false>;

    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    template <typename PosT>
    friend result_t visit_inner(this_t, PosT&& pos, size_t first)
    {
        auto idx    = pos.subindex(first);
        auto count  = pos.count();
        auto left_size  = pos.size_before(idx);
        auto child_size = pos.size_sbh(idx, left_size);
        auto dropped_size = first;
        auto child_dropped_size = dropped_size - left_size;
        if (Collapse && pos.shift() > BL && idx == pos.count() - 1) {
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
            return { pos.shift(), newn };
        }
    }

    template <typename PosT>
    friend result_t visit_leaf(this_t, PosT&& pos, size_t first)
    {
        auto n = node_t::copy_leaf(pos.node(), pos.index(first), pos.count());
        return { 0, n };
    };
};

template <typename Node>
struct concat_merger
{
    using node_t = Node;
    static constexpr auto B  = Node::bits;
    static constexpr auto BL = Node::bits_leaf;

    count_t* curr;
    count_t  n;

    concat_merger(count_t* counts, count_t countn)
        : curr{counts}, n{countn}
    {};

    node_t* result    = node_t::make_inner_r_n(std::min(n, branches<B>));
    node_t* parent    = result;
    size_t  size_sum  = {};

    node_t* to        = {};
    count_t to_offset = {};
    size_t  to_size   = {};

    void add_child(node_t* p, size_t size)
    {
        ++curr;
        auto r = parent->relaxed();
        if (r->count == branches<B>) {
            assert(result == parent);
            auto new_parent = node_t::make_inner_r_n(n - branches<B>);
            result = node_t::make_inner_r_n(2u, parent, size_sum, new_parent);
            parent = new_parent;
            r = new_parent->relaxed();
            size_sum = 0;
        }
        auto idx = r->count++;
        r->sizes[idx] = size_sum += size;
        parent->inner() [idx] = p;
    };

    template <typename Pos>
    void merge_leaf(Pos&& p)
    {
        auto from       = p.node();
        auto from_size  = p.size();
        auto from_count = p.count();
        assert(from_size);
        if (!to && *curr == from_count) {
            add_child(from->inc(), from_size);
        } else {
            auto from_offset = count_t{};
            auto from_data   = from->leaf();
            do {
                if (!to) {
                    to = node_t::make_leaf_n(*curr);
                    to_offset = 0;
                }
                auto data = to->leaf();
                auto to_copy = std::min(from_count - from_offset,
                                        *curr - to_offset);
                std::uninitialized_copy(from_data + from_offset,
                                        from_data + from_offset + to_copy,
                                        data + to_offset);
                to_offset   += to_copy;
                from_offset += to_copy;
                if (*curr == to_offset) {
                    add_child(to, to_offset);
                    to = nullptr;
                }
            } while (from_offset != from_count);
        }
    }

    template <typename Pos>
    void merge_inner(Pos&& p)
    {
        auto from       = p.node();
        auto from_size  = p.size();
        auto from_count = p.count();
        assert(from_size);
        if (!to && *curr == from_count) {
            add_child(from->inc(), from_size);
        } else {
            auto from_offset = count_t{};
            auto from_data  = from->inner();
            do {
                if (!to) {
                    to = node_t::make_inner_r_n(*curr);
                    to_offset = 0;
                    to_size   = 0;
                }
                auto data    = to->inner();
                auto to_copy = std::min(from_count - from_offset,
                                        *curr - to_offset);
                std::uninitialized_copy(from_data + from_offset,
                                        from_data + from_offset + to_copy,
                                        data + to_offset);
                node_t::inc_nodes(from_data + from_offset, to_copy);
                auto sizes   = to->relaxed()->sizes;
                p.copy_sizes(from_offset, to_copy,
                             to_size, sizes + to_offset);
                to_offset   += to_copy;
                from_offset += to_copy;
                to_size      = sizes[to_offset - 1];
                if (*curr == to_offset) {
                    to->relaxed()->count = to_offset;
                    add_child(to, to_size);
                    to = nullptr;
                }
            } while (from_offset != from_count);
        }
    }

    relaxed_pos<Node> finish(shift_t shift, bool is_top)
    {
        assert(!to);
        if (parent != result) {
            assert(result->relaxed()->count == 2);
            auto r = result->relaxed();
            r->sizes[1] = r->sizes[0] + size_sum;
            return { result, shift + B, r };
        } else if (is_top) {
            return { result, shift, result->relaxed() };
        } else {
            auto n = node_t::make_inner_r_n(1u, result, size_sum);
            return { n, shift + B, n->relaxed() };
        }
    }
};

struct concat_merger_visitor
{
    using this_t = concat_merger_visitor;

    template <typename Pos, typename Merger>
    friend void visit_inner(this_t, Pos&& p, Merger& merger)
    { merger.merge_inner(p); }

    template <typename Pos, typename Merger>
    friend void visit_leaf(this_t, Pos&& p, Merger& merger)
    { merger.merge_leaf(p); }
};

struct concat_rebalance_plan_fill_visitor
{
    using this_t = concat_rebalance_plan_fill_visitor;

    template <typename Pos, typename Plan>
    friend void visit_node(this_t, Pos&& p, Plan& plan)
    {
        auto count = p.count();
        plan.counts[plan.n++] = count;
        plan.total += count;
    }
};

template <bits_t B, bits_t BL>
struct concat_rebalance_plan
{
    static constexpr auto max_children = 2 * branches<B>;

    count_t counts [max_children];
    count_t n = 0u;
    count_t total = 0u;

    template <typename LPos, typename CPos, typename RPos>
    void fill(LPos&& lpos, CPos&& cpos, RPos&& rpos)
    {
        assert(n == 0u);
        assert(total == 0u);
        using visitor_t = concat_rebalance_plan_fill_visitor;
        lpos.each_left_sub(visitor_t{}, *this);
        cpos.each_sub(visitor_t{}, *this);
        rpos.each_right_sub(visitor_t{}, *this);
    }

    void shuffle(shift_t shift)
    {
        constexpr auto rrb_extras    = 2u;
        constexpr auto rrb_invariant = 1u;
        const auto bits     = shift == BL ? BL : B;
        const auto branches = count_t{1} << bits;
        const auto optimal  = ((total - 1) >> bits) + 1;
        auto i = 0u;
        while (n >= optimal + rrb_extras) {
            // skip ok nodes
            while (counts[i] > branches - rrb_invariant) i++;
            // short node, redistribute
            auto remaining = counts[i];
            do {
                auto count = std::min(remaining + counts[i+1], branches);
                counts[i] = count;
                remaining +=  counts[i + 1] - count;
                ++i;
            } while (remaining > 0);
            // remove node
            std::move(counts + i + 1, counts + n, counts + i);
            --n;
            --i;
        }
    }

    template <typename LPos, typename CPos, typename RPos>
    relaxed_pos<node_type<CPos>>
    merge(LPos&& lpos, CPos&& cpos, RPos&& rpos, bool is_top)
    {
        using node_t    = node_type<CPos>;
        using merger_t  = concat_merger<node_t>;
        using visitor_t = concat_merger_visitor;
        auto merger = merger_t{counts, n};
        lpos.each_left_sub(visitor_t{}, merger);
        cpos.each_sub(visitor_t{}, merger);
        rpos.each_right_sub(visitor_t{}, merger);
        return merger.finish(cpos.shift(), is_top);
    }
};

template <typename Node, typename LPos, typename CPos, typename RPos>
relaxed_pos<Node>
concat_rebalance(LPos&& lpos, CPos&& cpos, RPos&& rpos, bool is_top)
{
    auto plan = concat_rebalance_plan<Node::bits, Node::bits_leaf>{};
    plan.fill(lpos, cpos, rpos);
    plan.shuffle(cpos.shift());
    return plan.merge(lpos, cpos, rpos, is_top);
}

template <typename Node, typename LPos, typename RPos>
relaxed_pos<Node>
concat_leafs(LPos&& lpos, RPos&& rpos, bool is_top)
{
    using node_t = Node;
    assert(!is_top);
    assert(lpos.shift() == rpos.shift());
    assert(lpos.shift() == 0);
    auto lcount = lpos.count();
    auto rcount = rpos.count();
    auto node   = node_t::make_inner_r_n(2u,
                                         lpos.node()->inc(), lcount,
                                         rpos.node()->inc(), rcount);
    return { node, node_t::bits_leaf, node->relaxed() };
}

template <typename Node>
struct concat_left_visitor;
template <typename Node>
struct concat_right_visitor;
template <typename Node>
struct concat_both_visitor;

template <typename Node, typename LPos, typename RPos>
relaxed_pos<Node>
concat_inners(LPos&& lpos, RPos&& rpos, bool is_top)
{
    auto lshift = lpos.shift();
    auto rshift = rpos.shift();
    if (lshift > rshift) {
        auto cpos = lpos.last_sub(concat_left_visitor<Node>{}, rpos, false);
        auto r = concat_rebalance<Node>(lpos, cpos, null_sub_pos{}, is_top);
        cpos.visit(dec_visitor{});
        return r;
    } else if (lshift < rshift) {
        auto cpos = rpos.first_sub(concat_right_visitor<Node>{}, lpos, false);
        auto r = concat_rebalance<Node>(null_sub_pos{}, cpos, rpos, is_top);
        cpos.visit(dec_visitor{});
        return r;
    } else {
        assert(lshift == rshift);
        assert(Node::bits_leaf == 0u || lshift > 0);
        auto cpos = lpos.last_sub(concat_both_visitor<Node>{}, rpos, false);
        auto r = concat_rebalance<Node>(lpos, cpos, rpos, is_top);
        cpos.visit(dec_visitor{});
        return r;
    }
}

template <typename Node>
struct concat_left_visitor
{
    using this_t = concat_left_visitor;

    template <typename LPos, typename RPos>
    friend relaxed_pos<Node>
    visit_inner(this_t, LPos&& lpos, RPos&& rpos, bool is_top)
    { return concat_inners<Node>(lpos, rpos, is_top); }

    template <typename LPos, typename RPos>
    friend relaxed_pos<Node>
    visit_leaf(this_t, LPos&& lpos, RPos&& rpos, bool is_top)
    { IMMER_UNREACHABLE; }
};

template <typename Node>
struct concat_right_visitor
{
    using this_t = concat_right_visitor;

    template <typename RPos, typename LPos>
    friend relaxed_pos<Node>
    visit_inner(this_t, RPos&& rpos, LPos&& lpos, bool is_top)
    { return concat_inners<Node>(lpos, rpos, is_top); }

    template <typename RPos, typename LPos>
    friend relaxed_pos<Node>
    visit_leaf(this_t, RPos&& rpos, LPos&& lpos, bool is_top)
    { return concat_leafs<Node>(lpos, rpos, is_top); }
};

template <typename Node>
struct concat_both_visitor
{
    using this_t = concat_both_visitor;

    template <typename LPos, typename RPos>
    friend relaxed_pos<Node>
    visit_inner(this_t, LPos&& lpos, RPos&& rpos, bool is_top)
    { return rpos.first_sub(concat_right_visitor<Node>{}, lpos, is_top); }

    template <typename LPos, typename RPos>
    friend relaxed_pos<Node>
    visit_leaf(this_t, LPos&& lpos, RPos&& rpos, bool is_top)
    { return rpos.first_sub_leaf(concat_right_visitor<Node>{}, lpos, is_top); }
};

template <typename Node>
struct concat_trees_right_visitor
{
    using this_t = concat_trees_right_visitor;

    template <typename RPos, typename LPos>
    friend auto visit_node(this_t, RPos&& rpos, LPos&& lpos)
    { return concat_inners<Node>(lpos, rpos, true); }
};

template <typename Node>
struct concat_trees_left_visitor
{
    using this_t = concat_trees_left_visitor;

    template <typename LPos, typename... Args>
    friend relaxed_pos<Node> visit_node(this_t, LPos&& lpos, Args&& ...args)
    { return visit_maybe_relaxed_sub(
            args...,
            concat_trees_right_visitor<Node>{},
            lpos); }
};

template <typename Node>
relaxed_pos<Node>
concat_trees(Node* lroot, shift_t lshift, size_t lsize,
             Node* rroot, shift_t rshift, size_t rsize)
{
    return visit_maybe_relaxed_sub(
            lroot, lshift, lsize,
            concat_trees_left_visitor<Node>{},
            rroot, rshift, rsize);
}

} // namespace rbts
} // namespace detail
} // namespace immer
