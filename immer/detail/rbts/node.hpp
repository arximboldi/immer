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
    using memory      = MemoryPolicy;
    using heap_policy = typename memory::heap;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;
    using value_t     = T;

    enum class kind_t
    {
        leaf,
        inner
    };

    struct meta_t
        : refs_t
        , ownee_t
    {};

    static constexpr bool has_meta      = !std::is_empty<meta_t>{};
    static constexpr bool embed_relaxed = memory::prefer_fewer_bigger_objects;

    struct relaxed_meta_t
    {
        meta_t      meta;
        count_t     count;
        std::size_t sizes[branches<B>];
    };

    struct relaxed_no_meta_t : meta_t
    {
        count_t     count;
        std::size_t sizes[branches<B>];
    };

    struct relaxed_embedded_t
    {
        count_t     count;
        std::size_t sizes[branches<B>];
    };

    using relaxed_t =
        std::conditional_t<embed_relaxed,
                           relaxed_embedded_t,
                           std::conditional_t<has_meta,
                                              relaxed_meta_t,
                                              relaxed_no_meta_t>>;

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

    using impl_t = std::conditional_t<has_meta,
                                      impl_meta_t,
                                      impl_no_meta_t>;

    static_assert(
        std::is_standard_layout<impl_t>::value,
        "payload must be of standard layout so we can use offsetof");

    impl_t impl;

    // assume that we need to keep headroom space in the node when we
    // are doing reference counting, since any node may become
    // transient when it has only one reference
    constexpr static bool keep_headroom = !std::is_empty<refs_t>{};

    constexpr static std::size_t sizeof_packed_leaf_n(count_t count)
    {
        return offsetof(impl_t, data.leaf.buffer)
            +  sizeof(leaf_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_packed_inner_n(count_t count)
    {
        return offsetof(impl_t, data.inner.buffer)
            +  sizeof(inner_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_packed_relaxed_n(count_t count)
    {
        return offsetof(relaxed_t, sizes)
            +  sizeof(size_t) * count;
    }

    constexpr static std::size_t sizeof_packed_inner_r_n(count_t count)
    {
        return embed_relaxed
            ? sizeof_packed_inner_n(count) + sizeof_packed_relaxed_n(count)
            : sizeof_packed_inner_n(count);
    }

    constexpr static std::size_t max_sizeof_leaf  =
        sizeof_packed_leaf_n(branches<BL>);

    constexpr static std::size_t max_sizeof_inner =
        sizeof_packed_inner_n(branches<B>);

    constexpr static std::size_t max_sizeof_relaxed =
        sizeof_packed_relaxed_n(branches<B>);

    constexpr static std::size_t max_sizeof_inner_r =
        sizeof_packed_inner_r_n(branches<B>);

    constexpr static std::size_t sizeof_inner_n(count_t n)
    { return keep_headroom ? max_sizeof_inner : sizeof_packed_inner_n(n); }

    constexpr static std::size_t sizeof_inner_r_n(count_t n)
    { return keep_headroom ? max_sizeof_inner_r : sizeof_packed_inner_r_n(n); }

    constexpr static std::size_t sizeof_relaxed_n(count_t n)
    { return keep_headroom ? max_sizeof_inner : sizeof_packed_relaxed_n(n); }

    constexpr static std::size_t sizeof_leaf_n(count_t n)
    { return keep_headroom ? max_sizeof_leaf : sizeof_packed_leaf_n(n); }

    using heap = typename heap_policy::template apply<
        max_sizeof_inner,
        max_sizeof_inner_r,
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

    template <typename U> friend auto meta_(U* x)       -> decltype(static_cast<meta_t&>(*x)) { return *x; }
    template <typename U> friend auto meta_(const U* x) -> decltype(static_cast<const meta_t&>(*x)) { return *x; }
    template <typename U> friend auto meta_(U* x)       -> decltype(static_cast<meta_t&>(x->meta)) { return x->meta; }
    template <typename U> friend auto meta_(const U* x) -> decltype(static_cast<const meta_t&>(x->meta)) { return x->meta; }
    template <typename U> friend auto meta_(U* x)       -> decltype(static_cast<meta_t&>(x->impl)) { return x->impl; }
    template <typename U> friend auto meta_(const U* x) -> decltype(static_cast<const meta_t&>(x->impl)) { return x->impl; }
    template <typename U> friend auto meta_(U* x)       -> decltype(static_cast<meta_t&>(x->impl.meta)) { return x->impl.meta; }
    template <typename U> friend auto meta_(const U* x) -> decltype(static_cast<const meta_t&>(x->impl.meta)) { return x->impl.meta; }

    template <typename U> static refs_t& refs(const U* x) { return const_cast<meta_t&>(meta_(x)); }
    template <typename U> static refs_t& refs(U* x) { return meta_(x); }

    template <typename U> static const ownee_t& ownee(const U* x) { return meta_(x); }
    template <typename U> static ownee_t& ownee(U* x) { return meta_(x); }

    static node_t* make_inner_n(count_t n)
    {
        assert(n <= branches<B>);
        auto m = check_alloc(heap::allocate(sizeof_inner_n(n)));
        auto p = new (m) node_t;
        p->impl.data.inner.relaxed = nullptr;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_e(edit_t e)
    {
        auto m = check_alloc(heap::allocate(max_sizeof_inner));
        auto p = new (m) node_t;
        ownee(p) = e;
        p->impl.data.inner.relaxed = nullptr;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_r_n(count_t n)
    {
        assert(n <= branches<B>);
        auto mp = check_alloc(heap::allocate(sizeof_inner_r_n(n)));
        auto mr = (void*){};
        if (embed_relaxed) {
            mr = reinterpret_cast<unsigned char*>(mp) + sizeof_inner_n(n);
        } else {
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

    static node_t* make_inner_sr_n(count_t n, relaxed_t* r)
    {
        return static_if<embed_relaxed, node_t*>(
            [&] (auto) {
                return node_t::make_inner_r_n(n);
            },
            [&] (auto) {
                auto p = new (check_alloc(heap::allocate(node_t::sizeof_inner_r_n(n)))) node_t;
                assert(r->count >= n);
                refs(r).inc();
                p->impl.data.inner.relaxed = r;
#if IMMER_RBTS_TAGGED_NODE
                p->impl.kind = node_t::kind_t::inner;
#endif
                return p;
            });
    }

    static node_t* make_inner_r_e(edit_t e)
    {
        auto mp = check_alloc(heap::allocate(max_sizeof_inner_r));
        auto mr = (void*){};
        if (embed_relaxed) {
            mr = reinterpret_cast<unsigned char*>(mp) + max_sizeof_inner;
        } else {
            try {
                mr = check_alloc(heap::allocate(max_sizeof_relaxed, norefs_tag{}));
            } catch (...) {
                heap::deallocate(mp);
                throw;
            }
        }
        auto p = new (mp) node_t;
        auto r = new (mr) relaxed_t;
        ownee(p) = e;
        static_if<!embed_relaxed>([&](auto){ ownee(r) = e; });
        r->count = 0u;
        p->impl.data.inner.relaxed = r;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_sr_e(edit_t e, relaxed_t* r)
    {
        return static_if<embed_relaxed, node_t*>(
            [&] (auto) {
                return node_t::make_inner_r_e(e);
            },
            [&] (auto) {
                auto p = new (check_alloc(heap::allocate(node_t::max_sizeof_inner_r))) node_t;
                refs(r).inc();
                p->impl.data.inner.relaxed = r;
                ownee(p) = e;
#if IMMER_RBTS_TAGGED_NODE
                p->impl.kind = node_t::kind_t::inner;
#endif
                return p;
            });
    }

    static node_t* make_leaf_n(count_t n)
    {
        assert(n <= branches<BL>);
        auto p = new (check_alloc(heap::allocate(sizeof_leaf_n(n)))) node_t;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::kind_t::leaf;
#endif
        return p;
    }

    static node_t* make_leaf_e(edit_t e)
    {
        auto p = new (check_alloc(heap::allocate(max_sizeof_leaf))) node_t;
        ownee(p) = e;
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

    static node_t* copy_inner_e(edit_t e, node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_e(e);
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

    static node_t* copy_inner_r_e(edit_t e, node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_r_e(e);
        return do_copy_inner_r(dst, src, n);
    }

    static node_t* copy_inner_sr_e(edit_t e, node_t* src, count_t n)
    {
        assert(src->kind() == kind_t::inner);
        auto dst = make_inner_sr_e(e, src->relaxed());
        return do_copy_inner_sr(dst, src, n);
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

    static node_t* do_copy_inner_sr(node_t* dst, node_t* src, count_t n)
    {
        if (embed_relaxed)
            return do_copy_inner_r(dst, src, n);
        else {
            inc_nodes(src->inner(), n);
            std::copy(src->inner(), src->inner() + n, dst->inner());
            return dst;
        }
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

    static node_t* copy_leaf_e(edit_t e, node_t* src, count_t idx, count_t last)
    {
        assert(src->kind() == kind_t::leaf);
        auto dst = make_leaf_e(e);
        try {
            std::uninitialized_copy(
                src->leaf() + idx, src->leaf() + last, dst->leaf());
        } catch (...) {
            heap::deallocate(dst);
            throw;
        }
        return dst;
    }

    static node_t* copy_leaf(node_t* src, count_t idx, count_t last)
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
        static_if<!embed_relaxed>([&] (auto) {
            if (refs(r).dec())
                heap::deallocate(r);
        });
        heap::deallocate(p);
    }

    static void delete_leaf(node_t* p, count_t n)
    {
        assert(p->kind() == kind_t::leaf);
        destroy_n(p->leaf(), n);
        heap::deallocate(p);
    }

    bool can_mutate(edit_t e) const
    {
        return refs(this).unique()
            || ownee(this).can_mutate(e);
    }

    relaxed_t* ensure_mutable_relaxed(edit_t e)
    {
        auto src_r = relaxed();
        return static_if<embed_relaxed, relaxed_t*>(
            [&] (auto) { return src_r; },
            [&] (auto) {
                if (refs(src_r).unique() || ownee(src_r).can_mutate(e))
                    return src_r;
                else {
                    return impl.data.inner.relaxed =
                        new (check_alloc(heap::allocate(max_sizeof_relaxed))) relaxed_t;
                }
            });
    }

    relaxed_t* ensure_mutable_relaxed_n(edit_t e, count_t n)
    {
        auto src_r = relaxed();
        return static_if<embed_relaxed, relaxed_t*>(
            [&] (auto) { return src_r; },
            [&] (auto) {
                if (refs(src_r).unique() || ownee(src_r).can_mutate(e))
                    return src_r;
                else {
                    auto dst_r =
                        new (check_alloc(heap::allocate(max_sizeof_relaxed))) relaxed_t;
                    std::copy(src_r->sizes, src_r->sizes + n, dst_r->sizes);
                    return impl.data.inner.relaxed = dst_r;
                }
            });
    }

    node_t* inc()
    {
        refs(this).inc();
        return this;
    }

    const node_t* inc() const
    {
        refs(this).inc();
        return this;
    }

    bool dec() const { return refs(this).dec(); }
    void dec_unsafe() const { refs(this).dec_unsafe(); }

    static void inc_nodes(node_t** p, count_t n)
    {
        for (auto i = p, e = i + n; i != e; ++i)
            refs(*i).inc();
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
    constexpr auto sizeof_elem = sizeof(node_t::leaf_t::buffer);
    constexpr auto space = node_t::max_sizeof_inner - node_t::sizeof_packed_leaf_n(0u);
    constexpr auto full_elems = space / sizeof_elem;
    constexpr auto BL = log2(full_elems);
    using result_t = node<T, MP, B, BL>;
    static_assert(
        result_t::max_sizeof_leaf <= result_t::max_sizeof_inner,
        "TODO: fix the heap policy design such that, in this case,\
we do not create free lists for degenerately big free nodes and\
always use the same sized free lists for all types!");
    return BL;
}

template <typename T, typename MP, bits_t B>
constexpr bits_t derive_bits_leaf = derive_bits_leaf_aux<T, MP, B>();

} // namespace rbts
} // namespace detail
} // namespace immer
