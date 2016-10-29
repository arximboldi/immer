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

#include <immer/heap/tags.hpp>
#include <immer/detail/rbts/bits.hpp>

#include <memory>
#include <cassert>

#ifdef NDEBUG
#define IMMER_RBTS_TAGGED_NODE 0
#else
#define IMMER_RBTS_TAGGED_NODE 1
#endif

namespace immer {
namespace detail {
namespace rbts {

template <typename T,
          bits_t   B,
          typename MemoryPolicy>
struct node
{
    static constexpr auto bits = B;

    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    enum class kind_t
    {
        leaf,
        inner
    };

    struct relaxed_t
    {
        count_t count;
        size_t  sizes[branches<B>];
    };

    struct leaf_t
    {
        T           items[branches<B>];
    };

    struct inner_t
    {
        relaxed_t*  relaxed;
        node*       children[branches<B>];
    };

    struct impl_t : refcount::data
    {
#if IMMER_RBTS_TAGGED_NODE
        kind_t kind;
#endif
        union data_t
        {
            inner_t inner;
            leaf_t  leaf;

            data_t() {}
            ~data_t() {}
        };

        data_t data;
    };

    using node_t = node;
    using heap   = typename heap_policy::template apply<
        sizeof(impl_t)
    >::type;

    impl_t impl;

#if IMMER_RBTS_TAGGED_NODE
    kind_t kind() const
    {
        return impl.kind;
    }
#endif

    relaxed_t* relaxed()
    {
        assert(kind() == kind_t::inner);
        return impl.data.inner.relaxed;
    }

    node_t** inner()
    {
        return impl.data.inner.children;
        assert(kind() == kind_t::inner);
    }

    T* leaf()
    {
        return impl.data.leaf.items;
        assert(kind() == kind_t::leaf);
    }

    constexpr static std::size_t sizeof_inner_n(count_t count)
    {
        return sizeof(node_t)
             - sizeof(node_t*) * (branches<B> - count);
    }

    constexpr static std::size_t sizeof_leaf_n(count_t count)
    {
        return sizeof(node_t)
             - sizeof(T) * (branches<B> - count);
    }

    static std::size_t sizeof_relaxed_n(count_t count)
    {
        return sizeof(relaxed_t)
             - sizeof(size_t) * (branches<B> - count);
    }

    static node_t* make_inner_n(count_t n)
    {
        auto p = new (heap::allocate(sizeof_inner_n(n))) node_t;
        p->impl.data.inner.relaxed = nullptr;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_r_n(count_t n)
    {
        node_t* p;
        relaxed_t* r;
        if (MemoryPolicy::prefer_fewer_bigger_objects) {
            p = new (heap::allocate(sizeof_inner_n(n) + sizeof_relaxed_n(n))) node_t;
            r = new (reinterpret_cast<unsigned char*>(p) + sizeof_inner_n(n)) relaxed_t;
        } else {
            p = new (heap::allocate(sizeof_inner_n(n))) node_t;
            r = new (heap::allocate(sizeof_relaxed_n(n), norefs_tag{})) relaxed_t;
        }
        r->count = 0u;
        p->impl.data.inner.relaxed = r;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_leaf_n(count_t n)
    {
        auto p = new (heap::allocate(sizeof_leaf_n(n))) node_t;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::leaf;
#endif
        return p;
    }

    static node_t* make_inner_n(count_t n, node_t* x)
    {
        assert(n >= 1u);
        auto p = make_inner_n(n);
        p->inner() [0] = x;
        return p;
    }

    static node_t* make_inner_n(count_t n, node_t* x, node_t* y)
    {
        assert(n >= 2u);
        auto p = make_inner_n(n);
        p->inner() [0] = x;
        p->inner() [1] = y;
        return p;
    }

    static node_t* make_inner_r_n(count_t n, node_t* x)
    {
        assert(n >= 1u);
        auto p = make_inner_r_n(n);
        auto r = p->relaxed();
        p->inner() [0] = x;
        r->count = 1;
        return p;
    }

    static node_t* make_inner_r_n(count_t n, node_t* x, size_t xs)
    {
        assert(n >= 1u);
        auto p = make_inner_r_n(n);
        auto r = p->relaxed();
        p->inner() [0] = x;
        r->sizes [0] = xs;
        r->count = 1;
        return p;
    }

    static node_t* make_inner_r_n(count_t n, node_t* x, node_t* y)
    {
        assert(n >= 2u);
        auto p = make_inner_r_n(n);
        auto r = p->relaxed();
        p->inner() [0] = x;
        p->inner() [1] = y;
        r->count = 2;
        return p;
    }

    static node_t* make_inner_r_n(count_t n,
                                  node_t* x, size_t xs,
                                  node_t* y)
    {
        assert(n >= 2u);
        auto p = make_inner_r_n(n);
        auto r = p->relaxed();
        p->inner() [0] = x;
        p->inner() [1] = y;
        r->sizes [0] = xs;
        r->count = 2;
        return p;
    }

    static node_t* make_inner_r_n(count_t n,
                                  node_t* x, size_t xs,
                                  node_t* y, size_t ys)
    {
        assert(n >= 2u);
        auto p = make_inner_r_n(n);
        auto r = p->relaxed();
        p->inner() [0] = x;
        p->inner() [1] = y;
        r->sizes [0] = xs;
        r->sizes [1] = xs + ys;
        r->count = 2;
        return p;
    }

    template <typename U>
    static node_t* make_leaf_n(count_t n, U&& x)
    {
        assert(n >= 1u);
        auto p = make_leaf_n(n);
        new (p->leaf()) T{ std::forward<U>(x) };
        return p;
    }

    static node_t* make_path(shift_t shift, node_t* node)
    {
        assert(node->kind() == kind_t::leaf);
        return shift == 0
            ? node
            : node_t::make_inner_n(1, make_path(shift - B, node));
    }

    static node_t* copy_inner(node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_n(n);
        inc_nodes(src->inner(), n);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        return dst;
    }

    static node_t* copy_inner_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_n(allocn);
        inc_nodes(src->inner(), n);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        return dst;
    }

    static node_t* copy_inner_r(node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_r_n(n);
        auto src_r = src->relaxed();
        auto dst_r = dst->relaxed();
        inc_nodes(src->inner(), n);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        std::uninitialized_copy(src_r->sizes, src_r->sizes + n, dst_r->sizes);
        dst_r->count = n;
        return dst;
    }

    static node_t* copy_inner_r_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_r_n(allocn);
        auto src_r = src->relaxed();
        auto dst_r = dst->relaxed();
        inc_nodes(src->inner(), n);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        std::uninitialized_copy(src_r->sizes, src_r->sizes + n, dst_r->sizes);
        dst_r->count = n;
        return dst;
    }

    static node_t* copy_leaf(node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(n);
        std::uninitialized_copy(src->leaf(), src->leaf() + n, dst->leaf());
        return dst;
    }

    static node_t* copy_leaf_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(allocn);
        std::uninitialized_copy(src->leaf(), src->leaf() + n, dst->leaf());
        return dst;
    }

    static node_t* copy_leaf(node_t* src1, count_t n1,
                             node_t* src2, count_t n2)
    {
        assert(src1->kind() == kind_t::leaf);
        assert(src2->kind() == kind_t::leaf);
        auto dst = make_leaf_n(n1 + n2);
        std::uninitialized_copy(
            src1->leaf(), src1->leaf() + n1, dst->leaf());
        std::uninitialized_copy(
            src2->leaf(), src2->leaf() + n2, dst->leaf() + n1);
        return dst;
    }

    static node_t* copy_leaf( node_t* src, int idx, int last)
    {
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(last - idx);
        std::uninitialized_copy(
            src->leaf() + idx, src->leaf() + last, dst->leaf());
        return dst;
    }

    template <typename U>
    static node_t* copy_leaf_emplace(node_t* src, count_t n, U&& x)
    {
        auto dst = copy_leaf_n(n + 1, src, n);
        new (dst->leaf() + n) T{std::forward<U>(x)};
        return dst;
    }

    static void delete_inner(node_t* p)
    {
        assert(p->kind() == kind_t::inner);
        assert(!p->relaxed());
        heap::deallocate(p);
    }

    static void delete_inner_r(node_t* p)
    {
        assert(p->kind() == kind_t::inner);
        auto r = p->relaxed();
        assert(r);
        if (!MemoryPolicy::prefer_fewer_bigger_objects)
            heap::deallocate(r);
        heap::deallocate(p);
    }

    static void delete_leaf(node_t* p, int n)
    {
        assert(p->kind() == kind_t::leaf);
        for (auto i = p->leaf(), e = i + n; i != e; ++i)
            i->~T();
        heap::deallocate(p);
    }

    node_t* inc()
    {
        refcount::inc(&impl);
        return this;
    }

    const node_t* inc() const
    {
        refcount::inc(&impl);
        return this;
    }

    bool dec() const
    {
        return refcount::dec(&impl);
    }

    void dec_unsafe() const
    {
        refcount::dec_unsafe(&impl);
    }

    static void inc_nodes(node_t** p, count_t n)
    {
        for (auto i = p, e = i + n; i != e; ++i)
            refcount::inc(&(*i)->impl);
    }

#if IMMER_RBTS_TAGGED_NODE
    shift_t compute_shift()
    {
        if (kind() == kind_t::leaf)
            return 0;
        else
            return B + inner() [0]->compute_shift();
    }
#endif

    bool check(shift_t shift, size_t size)
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(size > 0);
        if (shift == 0) {
            assert(kind() == kind_t::leaf);
            assert(size <= branches<B>);
        } else if (auto r = relaxed()) {
            auto count = r->count;
            assert(count > 0);
            assert(count <= branches<B>);
            if (r->sizes[count - 1] != size) {
                IMMER_TRACE_F("check");
                IMMER_TRACE_E(r->sizes[count - 1]);
                IMMER_TRACE_E(size);
            }
            assert(r->sizes[count - 1] == size);
            for (auto i = 1; i < count; ++i)
                assert(r->sizes[i - 1] < r->sizes[i]);
            auto last_size = size_t{};
            for (auto i = 0; i < count; ++i) {
                assert(inner()[i]->check(
                           shift - B,
                           r->sizes[i] - last_size));
                last_size = r->sizes[i];
            }
        } else {
            assert(size <= branches<B> << shift);
            auto count = (size >> shift)
                + (size - ((size >> shift) << shift) > 0);
            assert(count <= branches<B>);
            if (count) {
                for (auto i = 1; i < count - 1; ++i)
                    assert(inner()[i]->check(
                               shift - B,
                               1 << shift));
                assert(inner()[count - 1]->check(
                           shift - B,
                           size - ((count - 1) << shift)));
            }
        }
#endif // IMMER_DEBUG_DEEP_CHECK
        return true;
    }
};

} // namespace rbts
} // namespace detail
} // namespace immer
