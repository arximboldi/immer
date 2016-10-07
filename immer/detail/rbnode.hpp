
#pragma once

#include <memory>
#include <cassert>

#ifdef NDEBUG
#define IMMER_TAGGED_RBNODE 0
#else
#define IMMER_TAGGED_RBNODE 1
#endif

namespace immer {
namespace detail {

template <int B, typename T=std::size_t>
constexpr T branches = T{1} << B;

template <int B, typename T=std::size_t>
constexpr T mask = branches<B, T> - 1;

template <typename T,
          int B,
          typename MemoryPolicy>
struct rbnode
{
    using heap_policy = typename MemoryPolicy::heap;
    using refcount    = typename MemoryPolicy::refcount;

    enum kind_t
    {
        leaf_kind,
        inner_kind
    };

    struct impl_t : refcount::data
    {
#if IMMER_TAGGED_RBNODE
        kind_t kind;
#endif

        union data_t
        {
            using  leaf_t = T[branches<B>];
            struct inner_t
            {
                struct relaxed_t
                {
                    std::size_t sizes[branches<B>];
                    unsigned count = 0u;
                };
                rbnode*     elems[branches<B>];
                relaxed_t* relaxed;
            };

            inner_t inner;
            leaf_t  leaf;

            data_t() {}
            ~data_t() {}
        } data;
    };

    using node_t = rbnode;
    using heap   = typename heap_policy::template apply<
        sizeof(impl_t)
    >::type;

    impl_t impl;

#if IMMER_TAGGED_RBNODE
    kind_t kind() const
    {
        return impl.kind;
    }
#endif

    unsigned& slots()
    {
        assert(sizes());
        return impl.data.inner.relaxed->count;
    }

    const unsigned& slots() const
    {
        assert(sizes());
        return impl.data.inner.relaxed->count;
    }

    std::size_t* sizes()
    {
        assert(kind() == inner_kind);
        return reinterpret_cast<std::size_t*>(impl.data.inner.relaxed);
    }

    const std::size_t* sizes() const
    {
        assert(kind() == inner_kind);
        return impl.data.inner.relaxed;
    }

    node_t** inner()
    {
        assert(kind() == inner_kind);
        return impl.data.inner.elems;
    }

    const node_t* const* inner() const
    {
        assert(kind() == inner_kind);
        return impl.data.inner.elems;
    }

    T* leaf()
    {
        assert(kind() == leaf_kind);
        return impl.data.leaf;
    }

    const T* leaf() const
    {
        assert(kind() == leaf_kind);
        return impl.data.leaf;
    }

    static node_t* make_inner()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
        p->impl.data.inner.relaxed = nullptr;
#if IMMER_TAGGED_RBNODE
        p->impl.kind = node_t::inner_kind;
#endif
        return p;
    }

    static node_t* make_inner_r()
    {
        using relaxed_t = typename impl_t::data_t::inner_t::relaxed_t;
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
        auto r = new (heap::allocate(sizeof(relaxed_t))) relaxed_t;
        p->impl.data.inner.relaxed = r;
#if IMMER_TAGGED_RBNODE
        p->impl.kind = node_t::inner_kind;
#endif
        return p;
    }

    static node_t* make_leaf()
    {
        auto p = new (heap::allocate(sizeof(node_t))) node_t;
#if IMMER_TAGGED_RBNODE
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
        p->inner() [0] = x;
        p->slots() = 1;
        return p;
    }

    static node_t* make_inner_r(node_t* x, std::size_t xs)
    {
        auto p = make_inner_r();
        p->inner() [0] = x;
        p->sizes() [0] = xs;
        p->slots() = 1;
        return p;
    }

    static node_t* make_inner_r(node_t* x, node_t* y)
    {
        auto p = make_inner_r();
        p->inner() [0] = x;
        p->inner() [1] = y;
        p->slots() = 2;
        return p;
    }

    static node_t* make_inner_r(node_t* x, std::size_t xs, node_t* y)
    {
        auto p = make_inner_r();
        p->inner() [0] = x;
        p->inner() [1] = y;
        p->sizes() [0] = xs;
        p->slots() = 2;
        return p;
    }

    static node_t* make_inner_r(node_t* x, std::size_t xs,
                                node_t* y, std::size_t ys)
    {
        auto p = make_inner_r();
        p->inner() [0] = x;
        p->inner() [1] = y;
        p->sizes() [0] = xs;
        p->sizes() [1] = xs + ys;
        p->slots() = 2;
        return p;
    }

    template <typename U>
    static node_t* make_leaf(U&& x)
    {
        auto p = make_leaf();
        new (p->leaf()) T{ std::forward<U>(x) };
        return p;
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
        inc_nodes(src->inner(), n);
        std::uninitialized_copy(src->inner(), src->inner() + n, dst->inner());
        std::uninitialized_copy(src->sizes(), src->sizes() + n, dst->sizes());
        dst->slots() = n;
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
        auto sizes = p->sizes();
        if (sizes) heap::deallocate(sizes);
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
};

} // namespace detail
} // namespace immer
