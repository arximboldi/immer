
#pragma once

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
          int B,
          typename MemoryPolicy>
struct node
{
    static constexpr auto bits = B;

    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    enum kind_t
    {
        leaf_kind,
        inner_kind
    };

    struct relaxed_t
    {
        std::size_t sizes[branches<B>];
        unsigned    count;
    };

    struct leaf_t
    {
        T           items[branches<B>];
    };

    struct inner_t
    {
        node*     children[branches<B>];
        relaxed_t*  relaxed;
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
        assert(kind() == inner_kind);
        return impl.data.inner.relaxed;
    }

    node_t** inner()
    {
        assert(kind() == inner_kind);
        return impl.data.inner.children;
    }

    T* leaf()
    {
        assert(kind() == leaf_kind);
        return impl.data.leaf.items;
    }

    static node_t* make_inner()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
        p->impl.data.inner.relaxed = nullptr;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::inner_kind;
#endif
        return p;
    }

    static node_t* make_inner_r()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
        auto r = new (heap::allocate(sizeof(relaxed_t))) relaxed_t;
        r->count = 0u;
        p->impl.data.inner.relaxed = r;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::inner_kind;
#endif
        return p;
    }

    static node_t* make_leaf()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
#if IMMER_RBTS_TAGGED_NODE
        p->impl.kind = node_t::leaf_kind;
#endif
        return p;
    }

    static node_t* make_inner(node_t* x)
    {
        auto p = make_inner();
        p->inner() [0] = x;
        return p;
    }

    static node_t* make_inner(node_t* x, node_t* y)
    {
        auto p = make_inner();
        p->inner() [0] = x;
        p->inner() [1] = y;
        return p;
    }

    static node_t* make_inner_r(node_t* x)
    {
        auto p = make_inner_r();
        auto r = p->relaxed();
        p->inner() [0] = x;
        r->count = 1;
        return p;
    }

    static node_t* make_inner_r(node_t* x, std::size_t xs)
    {
        auto p = make_inner_r();
        auto r = p->relaxed();
        p->inner() [0] = x;
        r->sizes [0] = xs;
        r->count = 1;
        return p;
    }

    static node_t* make_inner_r(node_t* x, node_t* y)
    {
        auto p = make_inner_r();
        auto r = p->relaxed();
        p->inner() [0] = x;
        p->inner() [1] = y;
        r->count = 2;
        return p;
    }

    static node_t* make_inner_r(node_t* x, std::size_t xs, node_t* y)
    {
        auto p = make_inner_r();
        auto r = p->relaxed();
        p->inner() [0] = x;
        p->inner() [1] = y;
        r->sizes [0] = xs;
        r->count = 2;
        return p;
    }

    static node_t* make_inner_r(node_t* x, std::size_t xs,
                                node_t* y, std::size_t ys)
    {
        auto p = make_inner_r();
        auto r = p->relaxed();
        p->inner() [0] = x;
        p->inner() [1] = y;
        r->sizes [0] = xs;
        r->sizes [1] = xs + ys;
        r->count = 2;
        return p;
    }

    template <typename U>
    static node_t* make_leaf(U&& x)
    {
        auto p = make_leaf();
        new (p->leaf()) T{ std::forward<U>(x) };
        return p;
    }

    static node_t* make_path(unsigned level, node_t* node)
    {
        assert(node->kind() == node_t::leaf_kind);
        return level == 0
            ? node
            : node_t::make_inner(make_path(level - B, node));
    }

    static node_t* copy_inner(node_t* src, int n)
    {
        assert(src->kind() == node_t::inner_kind);
        auto dst = make_inner();
        inc_nodes(src->inner(), n);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        return dst;
    }

    static node_t* copy_inner_r(node_t* src, int n)
    {
        assert(src->kind() == node_t::inner_kind);
        auto dst = make_inner_r();
        auto src_r = src->relaxed();
        auto dst_r = dst->relaxed();
        inc_nodes(src->inner(), n);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        std::uninitialized_copy(src_r->sizes, src_r->sizes + n, dst_r->sizes);
        dst_r->count = n;
        return dst;
    }

    static node_t* copy_leaf(node_t* src, int n)
    {
        assert(src->kind() == node_t::leaf_kind);
        auto dst = make_leaf();
        std::uninitialized_copy(src->leaf(), src->leaf() + n, dst->leaf());
        return dst;
    }

    static node_t* copy_leaf(node_t* src1, int n1,
                             node_t* src2, int n2)
    {
        assert(src1->kind() == node_t::leaf_kind);
        assert(src2->kind() == node_t::leaf_kind);
        auto dst = make_leaf();
        std::uninitialized_copy(
            src1->leaf(), src1->leaf() + n1, dst->leaf());
        std::uninitialized_copy(
            src2->leaf(), src2->leaf() + n2, dst->leaf() + n1);
        return dst;
    }

    static node_t* copy_leaf(node_t* src, int idx, int last)
    {
        assert(src->kind() == node_t::leaf_kind);
        auto dst = make_leaf();
        std::uninitialized_copy(
            src->leaf() + idx, src->leaf() + last, dst->leaf());
        return dst;
    }

    template <typename U>
    static node_t* copy_leaf_emplace(node_t* src, int n, U&& x)
    {
        auto dst = copy_leaf(src, n);
        new (dst->leaf() + n) T{std::forward<U>(x)};
        return dst;
    }

    static void delete_inner(node_t* p)
    {
        assert(p->kind() == node_t::inner_kind);
        assert(!p->relaxed());
        heap::deallocate(p);
    }

    static void delete_inner_r(node_t* p)
    {
        assert(p->kind() == node_t::inner_kind);
        auto r = p->relaxed();
        assert(r);
        heap::deallocate(r);
        heap::deallocate(p);
    }

    static void delete_leaf(node_t* p, int n)
    {
        assert(p->kind() == node_t::leaf_kind);
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

    static void inc_nodes(node_t** p, unsigned n)
    {
        for (auto i = p, e = i + n; i != e; ++i)
            refcount::inc(&(*i)->impl);
    }

#if IMMER_RBTS_TAGGED_NODE
    unsigned compute_shift()
    {
        if (kind() == node_t::leaf_kind)
            return 0;
        else
            return B + inner() [0]->compute_shift();
    }
#endif

    bool check(unsigned shift, std::size_t size)
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(size > 0);
        if (shift == 0) {
            assert(kind() == node_t::leaf_kind);
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
            auto last_size = std::size_t{};
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
