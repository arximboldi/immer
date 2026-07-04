//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/bts/bits.hpp>
#include <immer/detail/combine_standard_layout.hpp>
#include <immer/detail/util.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

// Nodes are allocated at a size that may be smaller than sizeof(node)
// (the empty singleton, and whichever of the two kinds is smaller),
// which some GCC versions flag with false positives.  Same situation
// as in hamts/node.hpp.
#if !defined(_MSC_VER)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
#endif

namespace immer {
namespace detail {
namespace bts {

// A node of a B+-style (a,b)-tree: all values of type `T` live in the
// leaves, inner nodes hold copies of keys of type `K` acting as
// separators, plus children pointers and cumulative subtree sizes.
//
// The kind of a node (leaf or inner) is contextual: all leaves live
// at the same distance from the root, so it is known from the level
// of the traversal.  Debug builds carry a tag to assert this.
template <typename T, typename K, typename MemoryPolicy, bits_t B, bits_t BL>
struct node
{
    static_assert(B >= 2u && B <= 16u, "");
    static_assert(BL >= 1u && BL <= 16u, "");

    using node_t      = node;
    using memory      = MemoryPolicy;
    using heap_policy = typename memory::heap;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;
    using value_t     = T;
    using key_t       = K;

    enum class kind_t
    {
        leaf,
        inner
    };

    struct leaf_t
    {
        aligned_storage_for<T> buffer[branches<BL>];
    };

    struct inner_t
    {
        node_t* children[branches<B>];
        local_size_t sizes[branches<B>];
        aligned_storage_for<K> keys[branches<B> - 1u];
    };

    union data_t
    {
        leaf_t leaf;
        inner_t inner;
    };

    struct impl_data_t
    {
#if IMMER_TAGGED_NODE
        kind_t kind;
#endif
        count_t count;
        data_t data;
    };

    using impl_t = combine_standard_layout_t<impl_data_t, refs_t, ownee_t>;

    impl_t impl;

    constexpr static std::size_t sizeof_leaf_n(count_t count)
    {
        return immer_offsetof(impl_t, d.data) +
               sizeof(aligned_storage_for<T>) * count;
    }

    constexpr static std::size_t max_sizeof_leaf = sizeof_leaf_n(branches<BL>);

    constexpr static std::size_t max_sizeof_inner =
        immer_offsetof(impl_t, d.data) + sizeof(inner_t);

    // Nodes are always allocated at full capacity so that (a) the
    // heap can serve them from fixed-size free lists and (b) owned
    // nodes can grow in place during transient batches.
    using leaf_heap =
        typename heap_policy::template optimized<max_sizeof_leaf>::type;
    using inner_heap =
        typename heap_policy::template optimized<max_sizeof_inner>::type;

#if IMMER_TAGGED_NODE
    kind_t kind() const { return impl.d.kind; }
#endif

    count_t count() const { return impl.d.count; }

    T* values()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::leaf);
        return reinterpret_cast<T*>(impl.d.data.leaf.buffer);
    }

    const T* values() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::leaf);
        return reinterpret_cast<const T*>(impl.d.data.leaf.buffer);
    }

    K* keys()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return reinterpret_cast<K*>(impl.d.data.inner.keys);
    }

    const K* keys() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return reinterpret_cast<const K*>(impl.d.data.inner.keys);
    }

    node_t** children()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.children;
    }

    node_t* const* children() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.children;
    }

    local_size_t* sizes()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.sizes;
    }

    const local_size_t* sizes() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.sizes;
    }

    static refs_t& refs(const node_t* x)
    {
        return auto_const_cast(get<refs_t>(x->impl));
    }
    static const ownee_t& ownee(const node_t* x)
    {
        return get<ownee_t>(x->impl);
    }
    static ownee_t& ownee(node_t* x) { return get<ownee_t>(x->impl); }

    bool can_mutate(edit_t e) const
    {
        return refs(this).unique() || ownee(this).can_mutate(e);
    }

    static node_t* make_leaf_into(void* buffer, count_t n)
    {
        auto p = new (buffer) node_t;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = kind_t::leaf;
#endif
        p->impl.d.count = n;
        return p;
    }

    static node_t* make_leaf_n(count_t n)
    {
        assert(n <= branches<BL>);
        return make_leaf_into(leaf_heap::allocate(max_sizeof_leaf), n);
    }

    static node_t* make_inner_n(count_t n)
    {
        assert(n >= 1u && n <= branches<B>);
        auto p = new (inner_heap::allocate(max_sizeof_inner)) node_t;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = kind_t::inner;
#endif
        p->impl.d.count = n;
        return p;
    }

    static node_t* copy_leaf(node_t* src)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto n   = src->count();
        auto dst = make_leaf_n(n);
        IMMER_TRY {
            detail::uninitialized_copy(
                src->values(), src->values() + n, dst->values());
        }
        IMMER_CATCH (...) {
            deallocate_leaf(dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_inner(node_t* src)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto n   = src->count();
        auto dst = make_inner_n(n);
        IMMER_TRY {
            detail::uninitialized_copy(
                src->keys(), src->keys() + (n - 1u), dst->keys());
        }
        IMMER_CATCH (...) {
            deallocate_inner(dst);
            IMMER_RETHROW;
        }
        std::copy(src->children(), src->children() + n, dst->children());
        std::copy(src->sizes(), src->sizes() + n, dst->sizes());
        inc_nodes(src->children(), n);
        return dst;
    }

    static node_t* copy_leaf_replace(node_t* src, count_t idx, T v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto n = src->count();
        assert(idx < n);
        auto dst  = make_leaf_n(n);
        auto srcp = src->values();
        auto dstp = dst->values();
        IMMER_TRY {
            dstp = detail::uninitialized_copy(srcp, srcp + idx, dstp);
            new (dstp) T{std::move(v)};
            ++dstp;
            detail::uninitialized_copy(srcp + idx + 1u, srcp + n, dstp);
        }
        IMMER_CATCH (...) {
            detail::destroy(dst->values(), dstp);
            deallocate_leaf(dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_leaf_insert(node_t* src, count_t idx, T v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto n = src->count();
        assert(n < branches<BL>);
        assert(idx <= n);
        auto dst  = make_leaf_n(n + 1u);
        auto srcp = src->values();
        auto dstp = dst->values();
        IMMER_TRY {
            dstp = detail::uninitialized_copy(srcp, srcp + idx, dstp);
            new (dstp) T{std::move(v)};
            ++dstp;
            detail::uninitialized_copy(srcp + idx, srcp + n, dstp);
        }
        IMMER_CATCH (...) {
            detail::destroy(dst->values(), dstp);
            deallocate_leaf(dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    // distributes the values of a full leaf plus the value `v`
    // inserted at `idx` over two new leaves
    static std::pair<node_t*, node_t*>
    copy_leaf_split_insert(node_t* src, count_t idx, T v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto n = src->count();
        assert(n == branches<BL>);
        assert(idx <= n);
        auto total = n + 1u;
        auto lc    = total - total / 2u;
        auto rc    = total / 2u;
        auto srcp  = src->values();
        auto left  = static_cast<node_t*>(nullptr);
        auto right = static_cast<node_t*>(nullptr);
        if (idx < lc) {
            left      = make_leaf_n(lc);
            auto dstp = left->values();
            IMMER_TRY {
                dstp = detail::uninitialized_copy(srcp, srcp + idx, dstp);
                new (dstp) T{std::move(v)};
                ++dstp;
                detail::uninitialized_copy(srcp + idx, srcp + (lc - 1u), dstp);
            }
            IMMER_CATCH (...) {
                detail::destroy(left->values(), dstp);
                deallocate_leaf(left);
                IMMER_RETHROW;
            }
            IMMER_TRY {
                right = copy_leaf_range(src, lc - 1u, n, rc);
            }
            IMMER_CATCH (...) {
                delete_leaf(left);
                IMMER_RETHROW;
            }
        } else {
            left = copy_leaf_range(src, 0u, lc, lc);
            IMMER_TRY {
                right     = make_leaf_n(rc);
                auto dstp = right->values();
                IMMER_TRY {
                    dstp =
                        detail::uninitialized_copy(srcp + lc, srcp + idx, dstp);
                    new (dstp) T{std::move(v)};
                    ++dstp;
                    detail::uninitialized_copy(srcp + idx, srcp + n, dstp);
                }
                IMMER_CATCH (...) {
                    detail::destroy(right->values(), dstp);
                    deallocate_leaf(right);
                    IMMER_RETHROW;
                }
            }
            IMMER_CATCH (...) {
                delete_leaf(left);
                IMMER_RETHROW;
            }
        }
        return {left, right};
    }

    static node_t*
    copy_leaf_range(node_t* src, count_t first, count_t last, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        assert(last - first == n);
        auto dst  = make_leaf_n(n);
        auto srcp = src->values();
        IMMER_TRY {
            detail::uninitialized_copy(
                srcp + first, srcp + last, dst->values());
        }
        IMMER_CATCH (...) {
            deallocate_leaf(dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_inner_replace(node_t* src,
                                      count_t idx,
                                      node_t* child,
                                      local_size_t size_delta)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto n = src->count();
        assert(idx < n);
        auto dst  = make_inner_n(n);
        auto srck = src->keys();
        IMMER_TRY {
            detail::uninitialized_copy(srck, srck + (n - 1u), dst->keys());
        }
        IMMER_CATCH (...) {
            deallocate_inner(dst);
            IMMER_RETHROW;
        }
        auto srcc = src->children();
        std::copy(srcc, srcc + n, dst->children());
        dst->children()[idx] = child;
        auto srcs            = src->sizes();
        auto dsts            = dst->sizes();
        for (auto j = count_t{0}; j < n; ++j)
            dsts[j] = srcs[j] + (j >= idx ? size_delta : 0u);
        inc_nodes(srcc, idx);
        inc_nodes(srcc + idx + 1u, n - idx - 1u);
        return dst;
    }

    // replaces the child at `idx` with the two nodes `l` and `r`
    // separated by `sep`, in a node that still has room
    static node_t* copy_inner_insert_split(node_t* src,
                                           count_t idx,
                                           node_t* l,
                                           node_t* r,
                                           const key_t& sep,
                                           local_size_t size_l,
                                           local_size_t size_delta)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto n = src->count();
        assert(n < branches<B>);
        assert(idx < n);
        auto dst  = make_inner_n(n + 1u);
        auto srck = src->keys();
        auto dstk = dst->keys();
        IMMER_TRY {
            dstk = detail::uninitialized_copy(srck, srck + idx, dstk);
            new (dstk) key_t{sep};
            ++dstk;
            detail::uninitialized_copy(srck + idx, srck + (n - 1u), dstk);
        }
        IMMER_CATCH (...) {
            detail::destroy(dst->keys(), dstk);
            deallocate_inner(dst);
            IMMER_RETHROW;
        }
        auto srcc = src->children();
        auto dstc = dst->children();
        std::copy(srcc, srcc + idx, dstc);
        dstc[idx]      = l;
        dstc[idx + 1u] = r;
        std::copy(srcc + idx + 1u, srcc + n, dstc + idx + 2u);
        auto srcs = src->sizes();
        auto dsts = dst->sizes();
        std::copy(srcs, srcs + idx, dsts);
        dsts[idx] = (idx > 0u ? srcs[idx - 1u] : 0u) + size_l;
        for (auto j = idx + 1u; j <= n; ++j)
            dsts[j] = srcs[j - 1u] + size_delta;
        inc_nodes(srcc, idx);
        inc_nodes(srcc + idx + 1u, n - idx - 1u);
        return dst;
    }

    // replaces the child at `idx` of a full node with the two nodes
    // `l` and `r` separated by `sep`, distributing the result over
    // two new nodes; the separator between them is not stored: the
    // caller derives it from the smallest key of the right one
    static std::pair<node_t*, node_t*>
    copy_inner_split_insert(node_t* src,
                            count_t idx,
                            node_t* l,
                            node_t* r,
                            const key_t& sep,
                            local_size_t size_l,
                            local_size_t size_r)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto n = src->count();
        assert(n == branches<B>);
        assert(idx < n);
        auto srck = src->keys();
        auto srcc = src->children();
        auto srcs = src->sizes();

        const key_t* kp[branches<B>];
        node_t* cp[branches<B> + 1u];
        local_size_t sz[branches<B> + 1u];
        for (auto j = count_t{0}; j < idx; ++j) {
            kp[j] = srck + j;
            cp[j] = srcc[j];
            sz[j] = srcs[j] - (j > 0u ? srcs[j - 1u] : 0u);
        }
        kp[idx]      = &sep;
        cp[idx]      = l;
        sz[idx]      = size_l;
        cp[idx + 1u] = r;
        sz[idx + 1u] = size_r;
        for (auto j = idx + 1u; j < n; ++j)
            kp[j] = srck + (j - 1u);
        for (auto j = idx + 2u; j <= n; ++j) {
            cp[j] = srcc[j - 1u];
            sz[j] = srcs[j - 1u] - srcs[j - 2u];
        }

        auto total = n + 1u;
        auto lc    = total - total / 2u;
        auto rc    = total / 2u;
        auto left  = make_inner_keys_from(kp, 0u, lc - 1u, lc);
        auto right = static_cast<node_t*>(nullptr);
        IMMER_TRY {
            right = make_inner_keys_from(kp, lc, n, rc);
        }
        IMMER_CATCH (...) {
            delete_inner(left);
            IMMER_RETHROW;
        }
        auto fill = [](node_t* dst, node_t** c, local_size_t* s, count_t m) {
            auto acc = local_size_t{0};
            for (auto j = count_t{0}; j < m; ++j) {
                dst->children()[j] = c[j];
                acc += s[j];
                dst->sizes()[j] = acc;
            }
        };
        fill(left, cp, sz, lc);
        fill(right, cp + lc, sz + lc, rc);
        inc_nodes(srcc, idx);
        inc_nodes(srcc + idx + 1u, n - idx - 1u);
        return {left, right};
    }

    // makes an inner node of `n` children whose keys are copied from
    // `kp[first_key .. last_key)`; children and sizes are left
    // uninitialized, to be filled by the caller with nothrow code
    static node_t* make_inner_keys_from(const key_t* const* kp,
                                        count_t first_key,
                                        count_t last_key,
                                        count_t n)
    {
        assert(last_key - first_key == n - 1u);
        auto dst   = make_inner_n(n);
        auto dstk  = dst->keys();
        auto built = count_t{0};
        IMMER_TRY {
            for (auto j = first_key; j < last_key; ++j, ++built)
                new (dstk + built) key_t{*kp[j]};
        }
        IMMER_CATCH (...) {
            detail::destroy_n(dstk, built);
            deallocate_inner(dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* make_inner_2(node_t* l,
                                node_t* r,
                                const key_t& sep,
                                local_size_t size_l,
                                local_size_t size_r)
    {
        auto dst = make_inner_n(2u);
        IMMER_TRY {
            new (dst->keys()) key_t{sep};
        }
        IMMER_CATCH (...) {
            deallocate_inner(dst);
            IMMER_RETHROW;
        }
        dst->children()[0] = l;
        dst->children()[1] = r;
        dst->sizes()[0]    = size_l;
        dst->sizes()[1]    = size_l + size_r;
        return dst;
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

    static void inc_nodes(node_t* const* p, count_t n)
    {
        for (auto i = p, e = i + n; i != e; ++i)
            refs(*i).inc();
    }

    static void deallocate_leaf(node_t* p)
    {
        leaf_heap::deallocate(max_sizeof_leaf, p);
    }

    static void deallocate_inner(node_t* p)
    {
        inner_heap::deallocate(max_sizeof_inner, p);
    }

    static void delete_leaf(node_t* p)
    {
        assert(p);
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::leaf);
        detail::destroy_n(p->values(), p->count());
        deallocate_leaf(p);
    }

    static void delete_inner(node_t* p)
    {
        assert(p);
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::inner);
        detail::destroy_n(p->keys(), p->count() - 1u);
        deallocate_inner(p);
    }

    // level == 0 means `p` is a leaf
    static void delete_deep(node_t* p, count_t level)
    {
        if (level == 0u)
            delete_leaf(p);
        else {
            auto fst = p->children();
            auto lst = fst + p->count();
            for (; fst != lst; ++fst)
                if ((*fst)->dec())
                    delete_deep(*fst, level - 1u);
            delete_inner(p);
        }
    }
};

} // namespace bts
} // namespace detail
} // namespace immer

#if !defined(_MSC_VER)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif
