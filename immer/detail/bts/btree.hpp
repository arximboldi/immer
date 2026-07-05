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
#include <immer/detail/bts/node.hpp>
#include <immer/detail/type_traits.hpp>

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace immer {
namespace detail {
namespace bts {

#if IMMER_DEBUG_STATS
struct btree_debug_stats
{
    std::size_t bits{};
    std::size_t bits_leaf{};
    std::size_t value_size{};
    std::size_t key_size{};

    std::size_t leaf_count{};
    std::size_t inner_count{};
    std::size_t value_count{};
    std::size_t child_count{};
};
#endif

// A persistent (a,b)-tree with values in the leaves.
//
// Every leaf lives at distance `depth` from the root, so the kind of
// a node is always known from the context of the traversal.  Inner
// nodes hold `count() - 1` separator keys: `sep[i]` partitions the
// children such that `keys(child[i]) < sep[i] <= keys(child[i+1])` --
// a separator does not need to be present in the tree, which spares
// erasure from ever rewriting inner keys.
template <typename T,
          typename KeyFn,
          typename Compare,
          typename MemoryPolicy,
          bits_t B,
          bits_t BL>
struct btree
{
    static constexpr auto bits      = B;
    static constexpr auto bits_leaf = BL;

    using value_t   = T;
    using key_fn_t  = KeyFn;
    using compare_t = Compare;
    using key_t     = std::decay_t<decltype(KeyFn{}(std::declval<const T&>()))>;
    using node_t    = node<T, key_t, MemoryPolicy, B, BL>;
    using edit_t    = typename MemoryPolicy::transience_t::edit;
    using owner_t   = typename MemoryPolicy::transience_t::owner;

    // in-place manipulation of the values of owned leaves requires
    // that a torn operation can not leave a node half-written
    constexpr static auto inplace_values =
        std::is_nothrow_move_constructible<T>::value &&
        std::is_nothrow_move_assignable<T>::value;

    node_t* root;
    size_t size;
    count_t depth; // number of inner levels; 0 means the root is a leaf

    static node_t* empty_root()
    {
        static const auto empty_ = [] {
            constexpr auto size = node_t::sizeof_leaf_n(0u);
            static std::aligned_storage_t<size, alignof(node_t)> storage;
            return node_t::make_leaf_into(&storage, 0u);
        }();
        return empty_->inc();
    }

    static btree empty() { return {empty_root(), 0u, 0u}; }

    template <typename U>
    static auto from_initializer_list(std::initializer_list<U> values)
    {
        auto e      = owner_t{};
        auto result = btree{empty()};
        for (auto&& v : values)
            result.add_mut(e, v);
        return result;
    }

    template <typename Iter,
              typename Sent,
              std::enable_if_t<compatible_sentinel_v<Iter, Sent>, bool> = true>
    static auto from_range(Iter first, Sent last)
    {
        auto e      = owner_t{};
        auto result = btree{empty()};
        for (; first != last; ++first)
            result.add_mut(e, *first);
        return result;
    }

    btree(node_t* r, size_t sz, count_t d) noexcept
        : root{r}
        , size{sz}
        , depth{d}
    {
    }

    btree(const btree& other) noexcept
        : btree{other.root, other.size, other.depth}
    {
        inc();
    }

    btree(btree&& other) noexcept
        : btree{empty_root(), 0u, 0u}
    {
        swap(*this, other);
    }

    btree& operator=(const btree& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    btree& operator=(btree&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(btree& x, btree& y) noexcept
    {
        using std::swap;
        swap(x.root, y.root);
        swap(x.size, y.size);
        swap(x.depth, y.depth);
    }

    ~btree() { dec(); }

    void inc() const { root->inc(); }

    void dec() const
    {
        if (root->dec())
            node_t::delete_deep(root, depth);
    }

    // index of the child of `p` whose subtree may contain key `k`,
    // this is, the number of separators s such that `s <= k`
    template <typename Key>
    static count_t inner_index(const node_t* p, const Key& k)
    {
        auto keys = p->keys();
        auto lo   = count_t{0};
        auto hi   = p->count() - 1u;
        while (lo < hi) {
            auto mid = (lo + hi) / 2u;
            if (Compare{}(k, keys[mid]))
                hi = mid;
            else
                lo = mid + 1u;
        }
        return lo;
    }

    // index of the first value of leaf `p` whose key is not less
    // than `k`, which is `p->count()` when there is none
    template <typename Key>
    static count_t leaf_index(const node_t* p, const Key& k)
    {
        auto values = p->values();
        auto lo     = count_t{0};
        auto hi     = p->count();
        while (lo < hi) {
            auto mid = (lo + hi) / 2u;
            if (Compare{}(KeyFn{}(values[mid]), k))
                lo = mid + 1u;
            else
                hi = mid;
        }
        return lo;
    }

    // index of the first value of leaf `p` whose key is greater than
    // `k`, which is `p->count()` when there is none
    template <typename Key>
    static count_t leaf_index_upper(const node_t* p, const Key& k)
    {
        auto values = p->values();
        auto lo     = count_t{0};
        auto hi     = p->count();
        while (lo < hi) {
            auto mid = (lo + hi) / 2u;
            if (Compare{}(k, KeyFn{}(values[mid])))
                hi = mid;
            else
                lo = mid + 1u;
        }
        return lo;
    }

    template <typename Key>
    const node_t* leaf_for(const Key& k) const
    {
        auto p = root;
        for (auto level = depth; level > 0u; --level)
            p = p->children()[inner_index(p, k)];
        return p;
    }

    template <typename Project, typename Default, typename Key>
    decltype(auto) get(const Key& k) const
    {
        auto p   = leaf_for(k);
        auto idx = leaf_index(p, k);
        if (idx < p->count() && !Compare{}(k, KeyFn{}(p->values()[idx])))
            return Project{}(p->values()[idx]);
        else
            return Default{}();
    }

    // smallest key of the subtree rooted at `p`; used to derive the
    // separator for a freshly split node, which spares threading key
    // copies through the recursion
    static decltype(auto) first_key(const node_t* p, count_t level)
    {
        for (; level > 0u; --level)
            p = p->children()[0];
        return KeyFn{}(p->values()[0]);
    }

    static local_size_t subtree_size(const node_t* p, count_t level)
    {
        return level == 0u ? p->count() : p->sizes()[p->count() - 1u];
    }

    struct add_result
    {
        node_t* node;
        node_t* split; // when non-null, new right sibling of `node`
    };

    btree add(T v) const
    {
        auto added    = false;
        auto r        = do_add(root, depth, std::move(v), added);
        auto new_size = size + (added ? 1u : 0u);
        if (!r.split)
            return {r.node, new_size, depth};
        IMMER_TRY {
            auto new_root = node_t::make_inner_2(r.node,
                                                 r.split,
                                                 first_key(r.split, depth),
                                                 subtree_size(r.node, depth),
                                                 subtree_size(r.split, depth));
            return {new_root, new_size, depth + 1u};
        }
        IMMER_CATCH (...) {
            if (r.node->dec())
                node_t::delete_deep(r.node, depth);
            if (r.split->dec())
                node_t::delete_deep(r.split, depth);
            IMMER_RETHROW;
        }
    }

    add_result do_add(node_t* p, count_t level, T v, bool& added) const
    {
        if (level == 0u) {
            auto n   = p->count();
            auto idx = leaf_index(p, KeyFn{}(v));
            auto found =
                idx < n && !Compare{}(KeyFn{}(v), KeyFn{}(p->values()[idx]));
            if (found) {
                added = false;
                return {node_t::copy_leaf_replace(p, idx, std::move(v)),
                        nullptr};
            }
            added = true;
            if (n < branches<BL>)
                return {node_t::copy_leaf_insert(p, idx, std::move(v)),
                        nullptr};
            auto lr = node_t::copy_leaf_split_insert(p, idx, std::move(v));
            return {lr.first, lr.second};
        }
        auto idx = inner_index(p, KeyFn{}(v));
        auto r   = do_add(p->children()[idx], level - 1u, std::move(v), added);
        auto delta = static_cast<local_size_t>(added ? 1u : 0u);
        if (!r.split) {
            IMMER_TRY {
                return {node_t::copy_inner_replace(p, idx, r.node, delta),
                        nullptr};
            }
            IMMER_CATCH (...) {
                if (r.node->dec())
                    node_t::delete_deep(r.node, level - 1u);
                IMMER_RETHROW;
            }
        }
        IMMER_TRY {
            decltype(auto) sep = first_key(r.split, level - 1u);
            auto size_l        = subtree_size(r.node, level - 1u);
            if (p->count() < branches<B>)
                return {node_t::copy_inner_insert_split(
                            p, idx, r.node, r.split, sep, size_l, delta),
                        nullptr};
            auto size_r = subtree_size(r.split, level - 1u);
            auto lr     = node_t::copy_inner_split_insert(
                p, idx, r.node, r.split, sep, size_l, size_r);
            return {lr.first, lr.second};
        }
        IMMER_CATCH (...) {
            if (r.node->dec())
                node_t::delete_deep(r.node, level - 1u);
            if (r.split->dec())
                node_t::delete_deep(r.split, level - 1u);
            IMMER_RETHROW;
        }
    }

    struct add_mut_result
    {
        node_t* node;
        node_t* split; // when non-null, new right sibling of `node`
        bool mutated;  // `node` is the input node, edited in place
    };

    void add_mut(edit_t e, T v)
    {
        auto added = false;
        auto r     = do_add_mut(e, root, depth, std::move(v), added);
        if (r.split) {
            IMMER_TRY {
                auto new_root =
                    node_t::make_inner_2(r.node,
                                         r.split,
                                         first_key(r.split, depth),
                                         subtree_size(r.node, depth),
                                         subtree_size(r.split, depth));
                node_t::owned(new_root, e);
                if (root->dec())
                    node_t::delete_deep(root, depth);
                root = new_root;
                ++depth;
            }
            IMMER_CATCH (...) {
                if (r.node->dec())
                    node_t::delete_deep(r.node, depth);
                if (r.split->dec())
                    node_t::delete_deep(r.split, depth);
                IMMER_RETHROW;
            }
        } else if (!r.mutated) {
            auto old = root;
            root     = r.node;
            if (old->dec())
                node_t::delete_deep(old, depth);
        }
        size += added ? 1u : 0u;
    }

    add_mut_result
    do_add_mut(edit_t e, node_t* p, count_t level, T v, bool& added)
    {
        if (level == 0u) {
            auto n   = p->count();
            auto idx = leaf_index(p, KeyFn{}(v));
            auto found =
                idx < n && !Compare{}(KeyFn{}(v), KeyFn{}(p->values()[idx]));
            if (found) {
                added = false;
                if (p->can_mutate(e)) {
                    p->values()[idx] = std::move(v);
                    return {p, nullptr, true};
                }
                return {node_t::owned(
                            node_t::copy_leaf_replace(p, idx, std::move(v)), e),
                        nullptr,
                        false};
            }
            added = true;
            if (n < branches<BL>) {
                if (p->can_mutate(e)) {
                    auto done = detail::static_if<inplace_values, bool>(
                        [&](auto) {
                            node_t::insert_value_mut(p, idx, std::move(v));
                            return true;
                        },
                        [&](auto) { return false; });
                    if (done)
                        return {p, nullptr, true};
                }
                return {node_t::owned(
                            node_t::copy_leaf_insert(p, idx, std::move(v)), e),
                        nullptr,
                        false};
            }
            auto lr = node_t::copy_leaf_split_insert(p, idx, std::move(v));
            node_t::owned(lr.first, e);
            node_t::owned(lr.second, e);
            return {lr.first, lr.second, false};
        }
        if (!p->can_mutate(e)) {
            // in-place manipulation must stop for the whole subtree:
            // even a unique descendant may be shared transitively
            // through this shared node
            auto r = do_add(p, level, std::move(v), added);
            node_t::owned(r.node, e);
            if (r.split)
                node_t::owned(r.split, e);
            return {r.node, r.split, false};
        }
        auto idx   = inner_index(p, KeyFn{}(v));
        auto child = p->children()[idx];
        auto r     = do_add_mut(e, child, level - 1u, std::move(v), added);
        auto delta = static_cast<local_size_t>(added ? 1u : 0u);
        if (!r.split) {
            if (!r.mutated) {
                p->children()[idx] = r.node;
                if (child->dec())
                    node_t::delete_deep(child, level - 1u);
            }
            if (delta) {
                auto sizes = p->sizes();
                for (auto j = idx; j < p->count(); ++j)
                    sizes[j] += delta;
            }
            return {p, nullptr, true};
        }
        IMMER_TRY {
            decltype(auto) sep = first_key(r.split, level - 1u);
            auto size_l        = subtree_size(r.node, level - 1u);
            if (p->count() < branches<B>) {
                auto dst = node_t::owned(
                    node_t::copy_inner_insert_split(
                        p, idx, r.node, r.split, sep, size_l, delta),
                    e);
                return {dst, nullptr, false};
            }
            auto size_r = subtree_size(r.split, level - 1u);
            auto lr     = node_t::copy_inner_split_insert(
                p, idx, r.node, r.split, sep, size_l, size_r);
            node_t::owned(lr.first, e);
            node_t::owned(lr.second, e);
            return {lr.first, lr.second, false};
        }
        IMMER_CATCH (...) {
            if (r.node->dec())
                node_t::delete_deep(r.node, level - 1u);
            if (r.split->dec())
                node_t::delete_deep(r.split, level - 1u);
            IMMER_RETHROW;
        }
    }

    struct sub_result
    {
        node_t* node;   // nullptr when the key was not there
        bool underflow; // node fell under the minimum fill
    };

    template <typename Key>
    btree sub(const Key& k) const
    {
        auto r = do_sub(root, depth, k);
        if (!r.node)
            return *this;
        auto new_size = size - 1u;
        if (new_size == 0u) {
            if (r.node->dec())
                node_t::delete_deep(r.node, depth);
            return empty();
        }
        if (depth > 0u && r.node->count() == 1u) {
            auto child = r.node->children()[0]->inc();
            if (r.node->dec())
                node_t::delete_deep(r.node, depth);
            return {child, new_size, depth - 1u};
        }
        return {r.node, new_size, depth};
    }

    template <typename Key>
    sub_result do_sub(node_t* p, count_t level, const Key& k) const
    {
        if (level == 0u) {
            auto n   = p->count();
            auto idx = leaf_index(p, k);
            if (idx >= n || Compare{}(k, KeyFn{}(p->values()[idx])))
                return {nullptr, false};
            return {node_t::copy_leaf_erase(p, idx), n - 1u < min_branches<BL>};
        }
        auto idx = inner_index(p, k);
        auto r   = do_sub(p->children()[idx], level - 1u, k);
        if (!r.node)
            return {nullptr, false};
        auto child_level = level - 1u;
        auto dispose     = [&](node_t* q) {
            if (q->dec())
                node_t::delete_deep(q, child_level);
        };
        if (!r.underflow) {
            IMMER_TRY {
                return {node_t::copy_inner_replace(
                            p, idx, r.node, static_cast<local_size_t>(-1)),
                        false};
            }
            IMMER_CATCH (...) {
                dispose(r.node);
                IMMER_RETHROW;
            }
        }
        return do_sub_fix(p, level, idx, r.node);
    }

    // rebuilds `p` after an erasure left the fresh child meant for
    // position `idx` under the minimum fill, by merging it with a
    // neighbor or redistributing between them; consumes `fresh`
    sub_result
    do_sub_fix(node_t* p, count_t level, count_t idx, node_t* fresh) const
    {
        auto child_level = level - 1u;
        auto dispose     = [&](node_t* q) {
            if (q->dec())
                node_t::delete_deep(q, child_level);
        };
        auto left_idx = idx > 0u ? idx - 1u : idx;
        auto lhs      = idx > 0u ? p->children()[idx - 1u] : fresh;
        auto rhs      = idx > 0u ? fresh : p->children()[1u];
        auto combined = lhs->count() + rhs->count();
        auto cap      = child_level == 0u ? branches<BL> : branches<B>;
        if (combined <= cap) {
            auto merged = static_cast<node_t*>(nullptr);
            IMMER_TRY {
                merged = child_level == 0u
                             ? node_t::merge_leaves(lhs, rhs)
                             : node_t::merge_inners(
                                   lhs, rhs, first_key(rhs, child_level));
            }
            IMMER_CATCH (...) {
                dispose(fresh);
                IMMER_RETHROW;
            }
            IMMER_TRY {
                auto dst = node_t::copy_inner_merge(p, left_idx, merged);
                dispose(fresh);
                return {dst, p->count() - 1u < min_branches<B>};
            }
            IMMER_CATCH (...) {
                dispose(merged);
                dispose(fresh);
                IMMER_RETHROW;
            }
        } else {
            auto lr = std::pair<node_t*, node_t*>{nullptr, nullptr};
            IMMER_TRY {
                lr = child_level == 0u
                         ? node_t::balance_leaves(lhs, rhs)
                         : node_t::balance_inners(
                               lhs, rhs, first_key(rhs, child_level));
            }
            IMMER_CATCH (...) {
                dispose(fresh);
                IMMER_RETHROW;
            }
            IMMER_TRY {
                auto dst = node_t::copy_inner_replace_2(
                    p,
                    left_idx,
                    lr.first,
                    lr.second,
                    first_key(lr.second, child_level),
                    subtree_size(lr.first, child_level));
                dispose(fresh);
                return {dst, false};
            }
            IMMER_CATCH (...) {
                dispose(lr.first);
                dispose(lr.second);
                dispose(fresh);
                IMMER_RETHROW;
            }
        }
    }

    struct sub_mut_result
    {
        node_t* node;   // nullptr when the key was not there
        bool underflow; // node fell under the minimum fill
        bool mutated;   // `node` is the input node, edited in place
    };

    template <typename Key>
    void sub_mut(edit_t e, const Key& k)
    {
        auto r = do_sub_mut(e, root, depth, k);
        if (!r.node)
            return;
        if (!r.mutated) {
            auto old = root;
            root     = r.node;
            if (old->dec())
                node_t::delete_deep(old, depth);
        }
        --size;
        if (depth > 0u && root->count() == 1u) {
            auto old   = root;
            auto child = old->children()[0]->inc();
            root       = child;
            --depth;
            if (old->dec())
                node_t::delete_deep(old, depth + 1u);
        }
    }

    template <typename Key>
    sub_mut_result do_sub_mut(edit_t e, node_t* p, count_t level, const Key& k)
    {
        if (level == 0u) {
            auto n   = p->count();
            auto idx = leaf_index(p, k);
            if (idx >= n || Compare{}(k, KeyFn{}(p->values()[idx])))
                return {nullptr, false, false};
            auto underflow = n - 1u < min_branches<BL>;
            if (!underflow && p->can_mutate(e)) {
                auto done = detail::static_if<inplace_values, bool>(
                    [&](auto) {
                        node_t::erase_value_mut(p, idx);
                        return true;
                    },
                    [&](auto) { return false; });
                if (done)
                    return {p, false, true};
            }
            return {node_t::owned(node_t::copy_leaf_erase(p, idx), e),
                    underflow,
                    false};
        }
        if (!p->can_mutate(e)) {
            // in-place manipulation must stop for the whole subtree:
            // even a unique descendant may be shared transitively
            // through this shared node
            auto r = do_sub(p, level, k);
            if (!r.node)
                return {nullptr, false, false};
            node_t::owned(r.node, e);
            return {r.node, r.underflow, false};
        }
        auto idx   = inner_index(p, k);
        auto child = p->children()[idx];
        auto r     = do_sub_mut(e, child, level - 1u, k);
        if (!r.node)
            return {nullptr, false, false};
        if (!r.underflow) {
            if (!r.mutated) {
                p->children()[idx] = r.node;
                if (child->dec())
                    node_t::delete_deep(child, level - 1u);
            }
            auto sizes = p->sizes();
            for (auto j = idx; j < p->count(); ++j)
                sizes[j] -= 1u;
            return {p, false, true};
        }
        // an underflowing child is never one that was edited in
        // place, so `p` can be rebuilt as in the persistent case
        assert(!r.mutated);
        auto rr = do_sub_fix(p, level, idx, r.node);
        node_t::owned(rr.node, e);
        return {rr.node, rr.underflow, false};
    }

    // copy of the tree with the value under `k` replaced by the
    // combination of the key with `fn` applied to the projection of
    // the current value, or nullptr when the key is not there
    template <typename Project, typename Combine, typename Key, typename Fn>
    node_t*
    do_update_if_exists(node_t* p, count_t level, const Key& k, Fn&& fn) const
    {
        if (level == 0u) {
            auto n   = p->count();
            auto idx = leaf_index(p, k);
            if (idx >= n || Compare{}(k, KeyFn{}(p->values()[idx])))
                return nullptr;
            return node_t::copy_leaf_replace(
                p,
                idx,
                Combine{}(k,
                          std::forward<Fn>(fn)(Project{}(p->values()[idx]))));
        }
        auto idx = inner_index(p, k);
        auto res = do_update_if_exists<Project, Combine>(
            p->children()[idx], level - 1u, k, std::forward<Fn>(fn));
        if (!res)
            return nullptr;
        IMMER_TRY {
            return node_t::copy_inner_replace(p, idx, res, 0u);
        }
        IMMER_CATCH (...) {
            if (res->dec())
                node_t::delete_deep(res, level - 1u);
            IMMER_RETHROW;
        }
    }

    template <typename Project,
              typename Default,
              typename Combine,
              typename Key,
              typename Fn>
    btree update(const Key& k, Fn&& fn) const
    {
        auto res = do_update_if_exists<Project, Combine>(root, depth, k, fn);
        if (res)
            return {res, size, depth};
        return add(Combine{}(k, std::forward<Fn>(fn)(Default{}())));
    }

    template <typename Project, typename Combine, typename Key, typename Fn>
    btree update_if_exists(const Key& k, Fn&& fn) const
    {
        auto res = do_update_if_exists<Project, Combine>(
            root, depth, k, std::forward<Fn>(fn));
        return res ? btree{res, size, depth} : *this;
    }

    // as do_update_if_exists, but edits owned nodes in place; a
    // returned node equal to `p` means it was edited in place
    template <typename Project, typename Combine, typename Key, typename Fn>
    node_t* do_update_if_exists_mut(
        edit_t e, node_t* p, count_t level, const Key& k, Fn&& fn)
    {
        if (level == 0u) {
            auto n   = p->count();
            auto idx = leaf_index(p, k);
            if (idx >= n || Compare{}(k, KeyFn{}(p->values()[idx])))
                return nullptr;
            if (p->can_mutate(e)) {
                p->values()[idx] = Combine{}(
                    k, std::forward<Fn>(fn)(Project{}(p->values()[idx])));
                return p;
            }
            return node_t::owned(
                node_t::copy_leaf_replace(
                    p,
                    idx,
                    Combine{}(
                        k, std::forward<Fn>(fn)(Project{}(p->values()[idx])))),
                e);
        }
        if (!p->can_mutate(e)) {
            auto res = do_update_if_exists<Project, Combine>(
                p, level, k, std::forward<Fn>(fn));
            return res ? node_t::owned(res, e) : nullptr;
        }
        auto idx   = inner_index(p, k);
        auto child = p->children()[idx];
        auto res   = do_update_if_exists_mut<Project, Combine>(
            e, child, level - 1u, k, std::forward<Fn>(fn));
        if (!res)
            return nullptr;
        if (res != child) {
            p->children()[idx] = res;
            if (child->dec())
                node_t::delete_deep(child, level - 1u);
        }
        return p;
    }

    template <typename Project,
              typename Default,
              typename Combine,
              typename Key,
              typename Fn>
    void update_mut(edit_t e, const Key& k, Fn&& fn)
    {
        auto res =
            do_update_if_exists_mut<Project, Combine>(e, root, depth, k, fn);
        if (res) {
            if (res != root) {
                auto old = root;
                root     = res;
                if (old->dec())
                    node_t::delete_deep(old, depth);
            }
            return;
        }
        add_mut(e, Combine{}(k, std::forward<Fn>(fn)(Default{}())));
    }

    template <typename Project, typename Combine, typename Key, typename Fn>
    void update_if_exists_mut(edit_t e, const Key& k, Fn&& fn)
    {
        auto res = do_update_if_exists_mut<Project, Combine>(
            e, root, depth, k, std::forward<Fn>(fn));
        if (res && res != root) {
            auto old = root;
            root     = res;
            if (old->dec())
                node_t::delete_deep(old, depth);
        }
    }

    bool check_tree() const
    {
        auto ok = true;
        auto n  = do_check(root, depth, nullptr, nullptr, ok);
        return ok && n == size;
    }

    size_t do_check(const node_t* p,
                    count_t level,
                    const key_t* lo,
                    const key_t* hi,
                    bool& ok) const
    {
        auto in_bounds = [&](const key_t& k) {
            return (!lo || !Compare{}(k, *lo)) && (!hi || Compare{}(k, *hi));
        };
        if (level == 0u) {
            IMMER_ASSERT_TAGGED(p->kind() == node_t::kind_t::leaf);
            auto n = p->count();
            ok     = ok && n <= branches<BL>;
            ok     = ok && (p == root || n >= min_branches<BL>);
            auto v = p->values();
            for (auto i = count_t{0}; i < n; ++i) {
                ok = ok && in_bounds(KeyFn{}(v[i]));
                ok = ok && (i + 1u == n ||
                            Compare{}(KeyFn{}(v[i]), KeyFn{}(v[i + 1u])));
            }
            return n;
        } else {
            IMMER_ASSERT_TAGGED(p->kind() == node_t::kind_t::inner);
            auto n        = p->count();
            ok            = ok && n <= branches<B>;
            ok            = ok && (p == root ? n >= 2u : n >= min_branches<B>);
            auto keys     = p->keys();
            auto children = p->children();
            auto sizes    = p->sizes();
            for (auto i = count_t{0}; i + 1u < n; ++i) {
                ok = ok && in_bounds(keys[i]);
                ok = ok && (i + 2u == n || Compare{}(keys[i], keys[i + 1u]));
            }
            auto total = size_t{0};
            for (auto i = count_t{0}; i < n; ++i) {
                auto sub_lo = i == 0u ? lo : &keys[i - 1u];
                auto sub_hi = i + 1u == n ? hi : &keys[i];
                total += do_check(children[i], level - 1u, sub_lo, sub_hi, ok);
                ok = ok && sizes[i] == total;
            }
            return total;
        }
    }

#if IMMER_DEBUG_STATS
    void do_get_debug_stats(btree_debug_stats& stats,
                            const node_t* p,
                            count_t level) const
    {
        if (level == 0u) {
            ++stats.leaf_count;
            stats.value_count += p->count();
        } else {
            ++stats.inner_count;
            stats.child_count += p->count();
            auto fst = p->children();
            auto lst = fst + p->count();
            for (; fst != lst; ++fst)
                do_get_debug_stats(stats, *fst, level - 1u);
        }
    }

    btree_debug_stats get_debug_stats() const
    {
        auto stats       = btree_debug_stats{};
        stats.bits       = B;
        stats.bits_leaf  = BL;
        stats.value_size = sizeof(T);
        stats.key_size   = sizeof(key_t);
        do_get_debug_stats(stats, root, depth);
        return stats;
    }
#endif
};

} // namespace bts
} // namespace detail
} // namespace immer
