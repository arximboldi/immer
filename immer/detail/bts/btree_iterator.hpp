//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/bts/btree.hpp>
#include <immer/detail/iterator_facade.hpp>

#include <cassert>

namespace immer {
namespace detail {
namespace bts {

// A bidirectional iterator over a btree.  It walks a stack of
// (node, child offset) pairs from the root down to a cursor inside a
// leaf.  It does not own the nodes it points into: the tree it was
// built from must outlive it.  The one-past-the-end position is
// represented by a null cursor.
template <typename T,
          typename KeyFn,
          typename Compare,
          typename MemoryPolicy,
          bits_t B,
          bits_t BL>
struct btree_iterator
    : iterator_facade<btree_iterator<T, KeyFn, Compare, MemoryPolicy, B, BL>,
                      std::bidirectional_iterator_tag,
                      T,
                      const T&,
                      std::ptrdiff_t,
                      const T*>
{
    using tree_t = btree<T, KeyFn, Compare, MemoryPolicy, B, BL>;
    using node_t = typename tree_t::node_t;

    struct end_t
    {};
    struct lower_bound_t
    {};
    struct upper_bound_t
    {};

    btree_iterator() = default;

    btree_iterator(const tree_t& t)
        : depth_{t.depth}
    {
        path_[0] = t.root;
        if (t.size > 0u)
            descend_(0u, true);
    }

    btree_iterator(const tree_t& t, end_t)
        : depth_{t.depth}
    {
        path_[0] = t.root;
    }

    template <typename Key>
    btree_iterator(const tree_t& t, const Key& k, lower_bound_t)
        : depth_{t.depth}
    {
        position_(t, k, false);
    }

    template <typename Key>
    btree_iterator(const tree_t& t, const Key& k, upper_bound_t)
        : depth_{t.depth}
    {
        position_(t, k, true);
    }

private:
    friend iterator_core_access;

    const T* cur_                          = nullptr;
    const T* end_                          = nullptr;
    count_t depth_                         = 0;
    const node_t* path_[max_depth<B> + 1u] = {
        nullptr,
    };
    count_t off_[max_depth<B> + 1u] = {
        0,
    };

    // enters the first (or last) child at every level below `from`
    // and puts the cursor on the first (or last) value of the
    // reached leaf
    void descend_(count_t from, bool front)
    {
        for (auto l = from; l < depth_; ++l) {
            auto n        = path_[l];
            off_[l]       = front ? 0u : n->count() - 1u;
            path_[l + 1u] = n->children()[off_[l]];
        }
        auto leaf = path_[depth_];
        assert(leaf->count() > 0u);
        end_ = leaf->values() + leaf->count();
        cur_ = front ? leaf->values() : end_ - 1u;
    }

    template <typename Key>
    void position_(const tree_t& t, const Key& k, bool upper)
    {
        auto p   = t.root;
        path_[0] = p;
        for (auto l = count_t{0}; l < depth_; ++l) {
            off_[l]       = tree_t::inner_index(p, k);
            p             = p->children()[off_[l]];
            path_[l + 1u] = p;
        }
        auto idx =
            upper ? tree_t::leaf_index_upper(p, k) : tree_t::leaf_index(p, k);
        end_ = p->values() + p->count();
        cur_ = p->values() + idx;
        if (cur_ == end_)
            next_leaf_();
    }

    void increment()
    {
        assert(cur_);
        ++cur_;
        if (cur_ == end_)
            next_leaf_();
    }

    void next_leaf_()
    {
        auto l = depth_;
        while (l > 0u) {
            auto parent = path_[l - 1u];
            if (off_[l - 1u] + 1u < parent->count()) {
                ++off_[l - 1u];
                path_[l] = parent->children()[off_[l - 1u]];
                descend_(l, true);
                return;
            }
            --l;
        }
        cur_ = end_ = nullptr;
    }

    void decrement()
    {
        if (!cur_) {
            descend_(0u, false);
            return;
        }
        auto leaf = path_[depth_];
        if (cur_ != leaf->values()) {
            --cur_;
            return;
        }
        auto l = depth_;
        while (l > 0u) {
            if (off_[l - 1u] > 0u) {
                --off_[l - 1u];
                path_[l] = path_[l - 1u]->children()[off_[l - 1u]];
                descend_(l, false);
                return;
            }
            --l;
        }
        assert(false); // decrementing the begin iterator
    }

    bool equal(const btree_iterator& other) const { return cur_ == other.cur_; }

    const T& dereference() const { return *cur_; }
};

} // namespace bts
} // namespace detail
} // namespace immer
