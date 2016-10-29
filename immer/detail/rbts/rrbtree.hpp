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

#include <immer/config.hpp>
#include <immer/detail/rbts/node.hpp>
#include <immer/detail/rbts/position.hpp>
#include <immer/detail/rbts/algorithm.hpp>

#include <cassert>
#include <memory>
#include <numeric>

namespace immer {
namespace detail {
namespace rbts {

template <typename T,
          typename MemoryPolicy,
          bits_t   B,
          bits_t   BL>
struct rrbtree
{
    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    using node_t = node<T, MemoryPolicy, B, BL>;
    using heap   = typename node_t::heap;

    size_t  size;
    shift_t shift;
    node_t* root;
    node_t* tail;

    static const rrbtree empty;

    rrbtree(size_t sz, shift_t sh, node_t* r, node_t* t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {
        assert(check_tree());
    }

    rrbtree(const rrbtree& other)
        : rrbtree{other.size, other.shift, other.root, other.tail}
    {
        inc();
    }

    rrbtree(rrbtree&& other)
        : rrbtree{empty}
    {
        swap(*this, other);
    }

    rrbtree& operator=(const rrbtree& other)
    {
        auto next{other};
        swap(*this, next);
        return *this;
    }

    rrbtree& operator=(rrbtree&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(rrbtree& x, rrbtree& y)
    {
        using std::swap;
        swap(x.size,  y.size);
        swap(x.shift, y.shift);
        swap(x.root,  y.root);
        swap(x.tail,  y.tail);
    }

    ~rrbtree()
    {
        dec();
    }

    void inc() const
    {
        root->inc();
        tail->inc();
    }

    void dec() const
    {
        traverse(dec_visitor());
    }

    static void dec_node(node_t* n, shift_t shift, size_t size)
    {
        visit_maybe_relaxed_sub(n, shift, size, dec_visitor());
    }

    auto tail_size() const
    {
        return size - tail_offset();
    }

    auto tail_offset() const
    {
        auto r = root->relaxed();
        assert(r == nullptr || r->count);
        return
            r               ? r->sizes[r->count - 1] :
            size            ? (size - 1) & ~mask<B>
            /* otherwise */ : 0;
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off) visit_maybe_relaxed_sub(root, shift, tail_off, v, args...);
        else make_empty_regular_pos(root).visit(v, args...);

        if (tail_size) make_leaf_sub_pos(tail, tail_size).visit(v, args...);
        else make_empty_leaf_pos(tail).visit(v, args...);
    }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, size_t idx) const
    {
        auto tail_off  = tail_offset();
        return idx >= tail_off
            ? make_leaf_descent_pos(tail).visit(v, idx - tail_off)
            : visit_maybe_relaxed_descent(root, shift, v, idx);
    }

    template <typename Step, typename State>
    State reduce(Step step, State acc) const
    {
        traverse(reduce_visitor{}, step, acc);
        return acc;
    }

    std::tuple<shift_t, node_t*>
    push_tail(node_t* root, shift_t shift, size_t size,
              node_t* tail, count_t tail_size) const
    {
        if (auto r = root->relaxed()) {
            auto new_root = make_relaxed_pos(root, shift, r)
                .visit(push_tail_visitor<node_t>{}, tail, tail_size);
            if (new_root)
                return { shift, new_root };
            else {
                auto new_path = node_t::make_path(shift, tail);
                auto new_root = node_t::make_inner_r_n(2u,
                                                       root->inc(),
                                                       size,
                                                       new_path,
                                                       tail_size);
                return { shift + B, new_root };
            }
        } else if (size >> B >= 1u << shift) {
            auto new_path = node_t::make_path(shift, tail);
            auto new_root = node_t::make_inner_n(2u, root->inc(), new_path);
            return { shift + B, new_root };
        } else if (size) {
            auto new_root = make_regular_sub_pos(root, shift, size)
                .visit(push_tail_visitor<node_t>{}, tail);
            return { shift, new_root };
        } else {
            return { shift, node_t::make_path(shift, tail) };
        }
    }

    rrbtree push_back(T value) const
    {
        auto ts = tail_size();
        if (ts < branches<B>) {
            auto new_tail = node_t::copy_leaf_emplace(tail, ts,
                                                      std::move(value));
            return { size + 1, shift, root->inc(), new_tail };
        } else {
            using std::get;
            auto new_tail = node_t::make_leaf_n(1u, std::move(value));
            auto tail_off = tail_offset();
            auto new_root = push_tail(root, shift, tail_off,
                                      tail->inc(), size - tail_off);
            return { size + 1, get<0>(new_root), get<1>(new_root), new_tail };
        }
    }

    std::tuple<const T*, size_t, size_t>
    array_for(size_t idx) const
    {
        using std::get;
        auto tail_off = tail_offset();
        if (idx >= tail_off) {
            return { tail->leaf() + (idx - tail_off), tail_off, size };
        } else {
            auto subs = visit_maybe_relaxed_sub(
                root, shift, tail_off,
                relaxed_array_for_visitor<T>(), idx);
            auto offset = get<1>(subs);
            auto first  = idx - offset;
            auto end    = first + get<2>(subs);
            return { get<0>(subs) + offset, first, end };
        }
    }

    const T& get(size_t index) const
    {
        return descend(get_visitor<T>(), index);
    }

    template <typename FnT>
    rrbtree update(size_t idx, FnT&& fn) const
    {
        auto tail_off  = tail_offset();
        if (idx >= tail_off) {
            auto tail_size = size - tail_off;
            auto new_tail  = make_leaf_sub_pos(tail, tail_size)
                .visit(update_visitor<node_t>{}, idx - tail_off, fn);
            return { size, shift, root->inc(), new_tail };
        } else {
            auto new_root  = visit_maybe_relaxed_sub(
                root, shift, tail_off,
                update_visitor<node_t>{}, idx, fn);
            return { size, shift, new_root, tail->inc() };
        }
    }

    rrbtree assoc(size_t idx, T value) const
    {
        return update(idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    rrbtree take(size_t new_size) const
    {
        auto tail_off = tail_offset();
        if (new_size == 0) {
            return empty;
        } else if (new_size >= size) {
            return *this;
        } else if (new_size > tail_off) {
            auto new_tail = node_t::copy_leaf(tail, new_size - tail_off);
            return { new_size, shift, root->inc(), new_tail };
        } else {
            using std::get;
            auto l = new_size - 1;
            auto v = slice_right_visitor<node_t>();
            auto r = visit_maybe_relaxed_sub(root, shift, tail_off, v, l);
            auto new_shift = get<0>(r);
            auto new_root  = get<1>(r);
            auto new_tail  = get<3>(r);
            if (new_root) {
                assert(new_root->compute_shift() == get<0>(r));
                assert(new_root->check(new_shift, new_size - get<2>(r)));
                return { new_size, new_shift, new_root, new_tail };
            } else {
                return { new_size, B, empty.root->inc(), new_tail };
            }
        }
    }

    rrbtree drop(size_t elems) const
    {
        if (elems == 0) {
            return *this;
        } else if (elems >= size) {
            return empty;
        } else if (elems == tail_offset()) {
            return { size - elems, B, empty.root->inc(), tail->inc() };
        } else if (elems > tail_offset()) {
            auto tail_off = tail_offset();
            auto new_tail = node_t::copy_leaf(tail, elems - tail_off,
                                              size - tail_off);
            return { size - elems, B, empty.root->inc(), new_tail };
        } else {
            using std::get;
            auto v = slice_left_visitor<node_t>();
            auto r = visit_maybe_relaxed_sub(root, shift, tail_offset(), v, elems);
            auto new_root  = get<1>(r);
            auto new_shift = get<0>(r);
            return { size - elems, new_shift, new_root, tail->inc() };
        }
        return *this;
    }

    rrbtree concat(const rrbtree& r) const
    {
        using std::get;
        if (size == 0)
            return r;
        else if (r.size == 0)
            return *this;
        else if (r.tail_offset() == 0) {
            // just concat the tail, similar to push_back
            auto tail_offst = tail_offset();
            auto tail_size  = size - tail_offst;
            if (tail_size == branches<B>) {
                auto new_root = push_tail(root, shift, tail_offst,
                                          tail->inc(), tail_size);
                return { size + r.size, get<0>(new_root), get<1>(new_root),
                         r.tail->inc() };
            } else if (tail_size + r.size <= branches<B>) {
                auto new_tail = node_t::copy_leaf(tail, tail_size,
                                                  r.tail, r.size);
                return { size + r.size, shift, root->inc(), new_tail };
            } else {
                auto remaining = branches<B> - tail_size;
                auto add_tail  = node_t::copy_leaf(tail, tail_size,
                                                   r.tail, remaining);
                auto new_tail  = node_t::copy_leaf(r.tail, remaining, r.size);
                auto new_root  = push_tail(root, shift, tail_offst,
                                           add_tail, branches<B>);
                return { size + r.size, get<0>(new_root), get<1>(new_root),
                         new_tail };
            }
        } else {
            auto tail_offst = tail_offset();
            auto with_tail  = push_tail(root, shift, tail_offst,
                                        tail->inc(), size - tail_offst);
            auto lshift     = get<0>(with_tail);
            auto lroot      = get<1>(with_tail);
            assert(lroot->check(lshift, size));
            auto concated   = concat_trees(lroot, lshift, size,
                                           r.root, r.shift, r.tail_offset());
            auto new_shift  = concated.shift();
            auto new_root   = concated.node();
            assert(new_shift == new_root->compute_shift());
            assert(new_root->check(new_shift, size + r.tail_offset()));
            dec_node(lroot, lshift, size);
            return { size + r.size, new_shift, new_root, r.tail->inc() };
        }
    }

    bool check_tree() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(shift >= B);
        assert(tail_offset() <= size);
        assert(check_root());
        assert(check_tail());
#endif
        return true;
    }

    bool check_tail() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        if (tail_size() > 0)
            assert(tail->check(0, tail_size()));
#endif
        return true;
    }

    bool check_root() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        if (tail_offset() > 0)
            assert(root->check(shift, tail_offset()));
        else {
            assert(root->kind() == node_t::inner_kind);
            assert(shift == B);
        }
#endif
        return true;
    }

#if IMMER_DEBUG_PRINT
    void debug_print() const
    {
        std::cerr
            << "--" << std::endl
            << "{" << std::endl
            << "  size  = " << size << std::endl
            << "  shift = " << shift << std::endl
            << "  root  = " << std::endl;
        debug_print_node(root, shift, tail_offset());
        std::cerr << "  tail  = " << std::endl;
        debug_print_node(tail, 0, tail_size());
        std::cerr << "}" << std::endl;
    }

    void debug_print_indent(unsigned indent) const
    {
        while (indent --> 0)
            std::cerr << ' ';
    }

    void debug_print_node(node_t* node,
                          shift_t shift,
                          size_t size,
                          unsigned indent = 8) const
    {
        const auto indent_step = 4;

        if (shift == 0) {
            debug_print_indent(indent);
            std::cerr << "- {" << size << "} "
                      << pretty_print_array(node->leaf(), size)
                      << std::endl;
        } else if (auto r = node->relaxed()) {
            auto count = r->count;
            debug_print_indent(indent);
            std::cerr << "# {" << size << "} "
                      << pretty_print_array(r->sizes, r->count)
                      << std::endl;
            auto last_size = size_t{};
            for (auto i = 0; i < count; ++i) {
                debug_print_node(node->inner()[i],
                                 shift - B,
                                 r->sizes[i] - last_size,
                                 indent + indent_step);
                last_size = r->sizes[i];
            }
        } else {
            debug_print_indent(indent);
            std::cerr << "+ {" << size << "}" << std::endl;
            auto count = (size >> shift)
                + (size - ((size >> shift) << shift) > 0);
            if (count) {
                for (auto i = 0; i < count - 1; ++i)
                    debug_print_node(node->inner()[i],
                                     shift - B,
                                     1 << shift,
                                     indent + indent_step);
                debug_print_node(node->inner()[count - 1],
                                 shift - B,
                                 size - ((count - 1) << shift),
                                 indent + indent_step);
            }
        }
    }
#endif // IMMER_DEBUG_PRINT
};

template <typename T, typename MP, bits_t B, bits_t BL>
const rrbtree<T, MP, B, BL> rrbtree<T, MP, B, BL>::empty = {
    0,
    B,
    node_t::make_inner_n(0u),
    node_t::make_leaf_n(0u)
};

} // namespace rbts
} // namespace detail
} // namespace immer
