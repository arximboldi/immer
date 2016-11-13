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

#include <immer/detail/rbts/node.hpp>
#include <immer/detail/rbts/position.hpp>
#include <immer/detail/rbts/operations.hpp>

#include <cassert>
#include <memory>
#include <numeric>

namespace immer {
namespace detail {
namespace rbts {

template <typename T,
          typename MemoryPolicy,
          bits_t B,
          bits_t BL>
struct rbtree
{
    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    using node_t = node<T, MemoryPolicy, B, BL>;
    using heap   = typename node_t::heap;

    size_t   size;
    shift_t  shift;
    node_t*  root;
    node_t*  tail;

    static const rbtree empty;

    rbtree(size_t sz, shift_t sh, node_t* r, node_t* t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {
        assert(check_tree());
    }

    rbtree(const rbtree& other)
        : rbtree{other.size, other.shift, other.root, other.tail}
    {
        inc();
    }

    rbtree(rbtree&& other)
        : rbtree{empty}
    {
        swap(*this, other);
    }

    rbtree& operator=(const rbtree& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    rbtree& operator=(rbtree&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(rbtree& x, rbtree& y)
    {
        using std::swap;
        swap(x.size,  y.size);
        swap(x.shift, y.shift);
        swap(x.root,  y.root);
        swap(x.tail,  y.tail);
    }

    ~rbtree()
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

    auto tail_size() const
    {
        return size ? ((size - 1) & mask<BL>) + 1 : 0;
    }

    auto tail_offset() const
    {
        return size ? (size - 1) & ~mask<BL> : 0;
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off) make_regular_sub_pos(root, shift, tail_off).visit(v, args...);
        else make_empty_regular_pos(root).visit(v, args...);

        if (tail_size) make_leaf_sub_pos(tail, tail_size).visit(v, args...);
        else make_empty_leaf_pos(tail).visit(v, args...);
    }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, size_t idx) const
    {
        auto tail_off  = tail_offset();
        return idx >= tail_off
            ? make_leaf_descent_pos(tail).visit(v, idx)
            : visit_regular_descent(root, shift, v, idx);
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn) const
    {
        traverse(for_each_chunk_visitor{}, std::forward<Fn>(fn));
    }

    rbtree push_back(T value) const
    {
        auto tail_off = tail_offset();
        auto ts = size - tail_off;
        if (ts < branches<BL>) {
            auto new_tail = node_t::copy_leaf_emplace(tail, ts,
                                                      std::move(value));
            return { size + 1, shift, root->inc(), new_tail };
        } else {
            auto new_tail = node_t::make_leaf_n(1u, std::move(value));
            try {
                if (tail_off == size_t{branches<B>} << shift) {
                    auto new_root = node_t::make_inner_n(2u);
                    try {
                        auto path = node_t::make_path(shift, tail);
                        new_root->inner() [0] = root;
                        new_root->inner() [1] = path;
                        root->inc();
                        tail->inc();
                        return { size + 1, shift + B, new_root, new_tail };
                    } catch (...) {
                        node_t::delete_inner(new_root);
                        throw;
                    }
                } else if (tail_off) {
                    auto new_root = make_regular_sub_pos(root, shift, tail_off)
                        .visit(push_tail_visitor<node_t>{}, tail);
                    tail->inc();
                    return { size + 1, shift, new_root, new_tail };
                } else {
                    auto new_root = node_t::make_path(shift, tail);
                    tail->inc();
                    return { size + 1, shift, new_root, new_tail };
                }
            } catch (...) {
                node_t::delete_leaf(new_tail, 1u);
                throw;
            }
        }
    }

    const T* array_for(size_t index) const
    {
        return descend(array_for_visitor<T>(), index);
    }

    const T& get(size_t index) const
    {
        return descend(get_visitor<T>(), index);
    }

    template <typename FnT>
    rbtree update(size_t idx, FnT&& fn) const
    {
        auto tail_off  = tail_offset();
        if (idx >= tail_off) {
            auto tail_size = size - tail_off;
            auto new_tail  = make_leaf_sub_pos(tail, tail_size)
                .visit(update_visitor<node_t>{}, idx - tail_off, fn);
            return { size, shift, root->inc(), new_tail };
        } else {
            auto new_root  = make_regular_sub_pos(root, shift, tail_off)
                .visit(update_visitor<node_t>{}, idx, fn);
            return { size, shift, new_root, tail->inc() };
        }
    }

    rbtree assoc(size_t idx, T value) const
    {
        return update(idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    rbtree take(size_t new_size) const
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
            auto r = make_regular_sub_pos(root, shift, tail_off).visit(v, l);
            auto new_shift = get<0>(r);
            auto new_root  = get<1>(r);
            auto new_tail  = get<3>(r);
            if (new_root) {
                assert(new_root->compute_shift() == get<0>(r));
                assert(new_root->check(new_shift, new_size - get<2>(r)));
                return { new_size, new_shift, new_root, new_tail };
            } else {
                return { new_size, BL, empty.root->inc(), new_tail };
            }
        }
    }

    bool check_tree() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(shift >= BL);
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
            assert(root->kind() == node_t::kind_t::inner);
            assert(shift == BL);
        }
#endif
        return true;
    }
};

template <typename T, typename MP, bits_t B, bits_t BL>
const rbtree<T, MP, B, BL> rbtree<T, MP, B, BL>::empty = {
    0,
    BL,
    node_t::make_inner_n(0u),
    node_t::make_leaf_n(0u)
};

} // namespace rbts
} // namespace detail
} // namespace immer
