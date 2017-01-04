//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
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
#include <immer/detail/util.hpp>
#include <immer/detail/rbts/bits.hpp>

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>

#ifdef NDEBUG
#define IMMER_RBTS_TAGGED_NODE 0
#else
#define IMMER_RBTS_TAGGED_NODE 1
#endif

namespace immer {
namespace detail {
namespace rbts {

template <typename T>
using aligned_storage_for =
    typename std::aligned_storage<sizeof(T), alignof(T)>::type;

template <typename T,
          typename MemoryPolicy,
          bits_t   B,
          bits_t   BL>
struct node
{
    static constexpr auto bits      = B;
    static constexpr auto bits_leaf = BL;

    using node_t      = node;
    using heap_policy = typename MemoryPolicy::heap;
    using transience  = typename MemoryPolicy::transience;
    using refs_t      = typename MemoryPolicy::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;

    enum class kind_t
    {
        leaf,
        inner
    };

    struct relaxed_t
    {
        count_t     count;
        std::size_t sizes[branches<B>];
    };

    struct leaf_t
    {
        aligned_storage_for<T> buffer;
    };

    struct inner_t
    {
        relaxed_t*  relaxed;
        aligned_storage_for<node_t*> buffer;
    };

    union data_t
    {
        inner_t inner;
        leaf_t  leaf;
    };

    struct meta_t
        : refs_t
        , ownee_t
    {};

    struct impl_meta_t
    {
        meta_t meta;
#if IMMER_RBTS_TAGGED_NODE
        kind_t kind;
#endif
        data_t data;
    };

    struct impl_no_meta_t : meta_t
    {
#if IMMER_RBTS_TAGGED_NODE
        kind_t kind;
#endif
        data_t data;
    };

    using impl_t = std::conditional_t<
        std::is_empty<meta_t>{},
        impl_no_meta_t,
        impl_meta_t>;

    static_assert(
        std::is_standard_layout<impl_t>::value,
        "payload must be of standard layout so we can use offsetof");

    impl_t impl;

    constexpr static std::size_t sizeof_inner_n(count_t count)
    {
        return offsetof(impl_t, data.inner.buffer)
            +  sizeof(inner_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_leaf_n(count_t count)
    {
        return offsetof(impl_t, data.inner.buffer)
            +  sizeof(leaf_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_relaxed_n(count_t count)
    {
        return offsetof(relaxed_t, sizes)
            +  sizeof(size_t) * count;
    }

    constexpr static std::size_t max_sizeof_leaf  =
        sizeof_leaf_n(branches<BL>);

    constexpr static std::size_t max_sizeof_inner =
        MemoryPolicy::prefer_fewer_bigger_objects
        ? sizeof_inner_n(branches<B>) + sizeof_relaxed_n(branches<B>)
        : sizeof_inner_n(branches<B>);

    constexpr static std::size_t max_sizeof_relaxed =
        sizeof_relaxed_n(branches<B>);

    using heap = typename heap_policy::template apply<
        max_sizeof_inner,
        max_sizeof_leaf
    >::type;

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
        assert(kind() == kind_t::inner);
        return reinterpret_cast<node_t**>(&impl.data.inner.buffer);
    }

    T* leaf()
    {
        assert(kind() == kind_t::leaf);
        return reinterpret_cast<T*>(&impl.data.leaf.buffer);
    }

    static node_t* make_inner_n(count_t n)
    {
        assert(n <= branches<B>);
        auto m = detail::check_alloc(heap::allocate(sizeof_inner_n(n)));
        auto p = new (m) node_t;
        p->impl.data.inner.relaxed = nullptr;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_e(edit_t e)
    {
        auto m = detail::check_alloc(heap::allocate(max_sizeof_inner));
        auto p = new (m) node_t;
        p->ownee() = e;
        p->impl.data.inner.relaxed = nullptr;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_r_n(count_t n)
    {
        assert(n <= branches<B>);
        void *mp, *mr;
        if (MemoryPolicy::prefer_fewer_bigger_objects) {
            mp = check_alloc(heap::allocate(sizeof_inner_n(n) +
                                            sizeof_relaxed_n(n)));
            mr = reinterpret_cast<unsigned char*>(mp) + sizeof_inner_n(n);

        } else {
            mp = check_alloc(heap::allocate(sizeof_inner_n(n)));
            try {
                mr = check_alloc(heap::allocate(sizeof_relaxed_n(n), norefs_tag{}));
            } catch (...) {
                heap::deallocate(mp);
                throw;
            }
        }
        auto p = new (mp) node_t;
        auto r = new (mr) relaxed_t;
        r->count = 0u;
        p->impl.data.inner.relaxed = r;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_r_e(edit_t e)
    {
        void *mp, *mr;
        if (MemoryPolicy::prefer_fewer_bigger_objects) {
            mp = check_alloc(heap::allocate(max_sizeof_inner));
            mr = reinterpret_cast<unsigned char*>(mp) + max_sizeof_inner;

        } else {
            mp = check_alloc(heap::allocate(max_sizeof_inner));
            try {
                mr = check_alloc(heap::allocate(max_sizeof_relaxed, norefs_tag{}));
            } catch (...) {
                heap::deallocate(mp);
                throw;
            }
        }
        auto p = new (mp) node_t;
        auto r = new (mr) relaxed_t;
        r->count = 0u;
        p->ownee() = e;
        p->impl.data.inner.relaxed = r;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_leaf_n(count_t n)
    {
        assert(n <= branches<BL>);
        auto p = new (heap::allocate(sizeof_leaf_n(n))) node_t;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::leaf;
#endif
        return p;
    }

    static node_t* make_leaf_e(edit_t e)
    {
        auto p = new (heap::allocate(max_sizeof_leaf)) node_t;
        p->ownee() = e;
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

    static node_t* make_inner_n(edit_t n, node_t* x)
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
        try {
            new (p->leaf()) T{ std::forward<U>(x) };
        } catch (...) {
            heap::deallocate(p);
            throw;
        }
        return p;
    }

    template <typename U>
    static node_t* make_leaf_e(edit_t e, U&& x)
    {
        auto p = make_leaf_e(e);
        try {
            new (p->leaf()) T{ std::forward<U>(x) };
        } catch (...) {
            heap::deallocate(p);
            throw;
        }
        return p;
    }

    static node_t* make_path(shift_t shift, node_t* node)
    {
        assert(node->kind() == kind_t::leaf);
        if (shift == endshift<B, BL>)
            return node;
        else {
            auto n = node_t::make_inner_n(1);
            try {
                n->inner() [0] = make_path(shift - B, node);
            } catch (...) {
                heap::deallocate(n);
                throw;
            }
            return n;
        }
    }

    static node_t* make_path_e(edit_t e, shift_t shift, node_t* node)
    {
        assert(node->kind() == kind_t::leaf);
        if (shift == endshift<B, BL>)
            return node;
        else {
            auto n = node_t::make_inner_e(e);
            try {
                n->inner() [0] = make_path_e(e, shift - B, node);
            } catch (...) {
                heap::deallocate(n);
                throw;
            }
            return n;
        }
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
        return do_copy_inner(dst, src, n);
    }

    static node_t* do_copy_inner(node_t* dst, node_t* src, count_t n)
    {
        assert(dst->kind() == kind_t::inner);
        assert(src->kind() == kind_t::inner);
        auto p = src->inner();
        inc_nodes(p, n);
        std::uninitialized_copy(p, p + n, dst->inner());
        return dst;
    }

    static node_t* copy_inner_r(node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_r_n(n);
        return do_copy_inner_r(dst, src, n);
    }

    static node_t* copy_inner_r_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_r_n(allocn);
        return do_copy_inner_r(dst, src, n);
    }

    static node_t* do_copy_inner_r(node_t* dst, node_t* src, count_t n)
    {
        assert(dst->kind() == kind_t::inner);
        assert(src->kind() == kind_t::inner);
        auto src_r = src->relaxed();
        auto dst_r = dst->relaxed();
        inc_nodes(src->inner(), n);
        std::copy(src->inner(), src->inner() + n, dst->inner());
        std::copy(src_r->sizes, src_r->sizes + n, dst_r->sizes);
        dst_r->count = n;
        return dst;
    }

    static node_t* copy_leaf(node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(n);
        try {
            std::uninitialized_copy(src->leaf(), src->leaf() + n, dst->leaf());
        } catch (...) {
            heap::deallocate(dst);
            throw;
        }
        return dst;
    }

    static node_t* copy_leaf_e(edit_t e, node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_e(e);
        try {
            std::uninitialized_copy(src->leaf(), src->leaf() + n, dst->leaf());
        } catch (...) {
            heap::deallocate(dst);
            throw;
        }
        return dst;
    }

    static node_t* copy_leaf_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(allocn);
        try {
            std::uninitialized_copy(src->leaf(), src->leaf() + n, dst->leaf());
        } catch (...) {
            heap::deallocate(dst);
            throw;
        }
        return dst;
    }

    static node_t* copy_leaf(node_t* src1, count_t n1,
                             node_t* src2, count_t n2)
    {
        assert(src1->kind() == kind_t::leaf);
        assert(src2->kind() == kind_t::leaf);
        auto dst = make_leaf_n(n1 + n2);
        try {
            std::uninitialized_copy(
                src1->leaf(), src1->leaf() + n1, dst->leaf());
        } catch (...) {
            heap::deallocate(dst);
            throw;
        }
        try {
            std::uninitialized_copy(
                src2->leaf(), src2->leaf() + n2, dst->leaf() + n1);
        } catch (...) {
            destroy_n(dst->leaf(), n1);
            heap::deallocate(dst);
            throw;
        }
        return dst;
    }

    static node_t* copy_leaf( node_t* src, int idx, int last)
    {
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(last - idx);
        try {
            std::uninitialized_copy(
                src->leaf() + idx, src->leaf() + last, dst->leaf());
        } catch (...) {
            heap::deallocate(dst);
            throw;
        }
        return dst;
    }

    template <typename U>
    static node_t* copy_leaf_emplace(node_t* src, count_t n, U&& x)
    {
        auto dst = copy_leaf_n(n + 1, src, n);
        try {
            new (dst->leaf() + n) T{std::forward<U>(x)};
        } catch (...) {
            destroy_n(dst->leaf(), n);
            heap::deallocate(dst);
            throw;
        }
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

    static void delete_leaf(node_t* p, count_t n)
    {
        assert(p->kind() == kind_t::leaf);
        destroy_n(p->leaf(), n);
        heap::deallocate(p);
    }

    refs_t& refs() const
    {
        return reinterpret_cast<meta_t&>(const_cast<impl_t&>(impl));
    }

    const ownee_t& ownee() const
    {
        return reinterpret_cast<const meta_t&>(impl);
    }

    ownee_t& ownee()
    {
        return reinterpret_cast<meta_t&>(impl);
    }

    bool can_mutate() const
    {
        return refs().unique();
    }

    bool can_mutate(edit_t e) const
    {
        return can_mutate() || ownee().can_mutate(e);
    }

    node_t* inc()
    {
        refs().inc();
        return this;
    }

    const node_t* inc() const
    {
        refs().inc();
        return this;
    }

    bool dec() const { return refs().dec(); }
    void dec_unsafe() const { refs().dec_unsafe(); }

    static void inc_nodes(node_t** p, count_t n)
    {
        for (auto i = p, e = i + n; i != e; ++i)
            (*i)->refs().inc();
    }

#if IMMER_RBTS_TAGGED_NODE
    shift_t compute_shift()
    {
        if (kind() == kind_t::leaf)
            return endshift<B, BL>;
        else
            return B + inner() [0]->compute_shift();
    }
#endif

    bool check(shift_t shift, size_t size)
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(size > 0);
        if (shift == endshift<B, BL>) {
            assert(kind() == kind_t::leaf);
            assert(size <= branches<BL>);
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

template <typename T, typename MP, bits_t B>
constexpr bits_t derive_bits_leaf_aux()
{
    using node_t = node<T, MP, B, B>;
    auto sizeof_elem = sizeof(node_t::leaf_t::buffer);
    auto space = node_t::max_sizeof_inner - node_t::sizeof_leaf_n(0u);
    auto full_elems = space / sizeof_elem;
    return log2(full_elems);
}

template <typename T, typename MP, bits_t B>
constexpr bits_t derive_bits_leaf = derive_bits_leaf_aux<T, MP, B>();

} // namespace rbts
} // namespace detail
} // namespace immer
