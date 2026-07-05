//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/bts/btree.hpp>
#include <immer/detail/bts/btree_iterator.hpp>
#include <immer/memory_policy.hpp>

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace immer {

template <typename K,
          typename T,
          typename Compare,
          typename MemoryPolicy,
          detail::bts::bits_t B,
          detail::bts::bits_t BL>
class sorted_map_transient;

/*!
 * Immutable mapping of values from type `K` to type `T`, sorted by
 * keys according to `Compare`.
 *
 * @tparam K    The type of the keys.
 * @tparam T    The type of the values to be stored in the container.
 * @tparam Compare The type of a function object capable of comparing
 *              values of type `K`.  It must induce a strict weak
 *              ordering and be stateless and default constructible.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *              memory_policy.
 *
 * @rst
 *
 * This container provides a good trade-off between cache locality,
 * search, update performance and structural sharing.  It does so by
 * storing the data in contiguous chunks of :math:`2^{BL}` elements
 * ordered by key, indexed by inner nodes of :math:`2^{B}` children.
 * When storing big objects, the size of these contiguous chunks can
 * become too big, damaging performance.  If this is measured to be
 * problematic for a specific use-case, it can be solved by using a
 * `immer::box` to wrap the type `T`.
 *
 * In contrast to `immer::map`, iteration happens in the order defined
 * by `Compare`, and the container supports order based queries like
 * `lower_bound` and `upper_bound`.  In exchange, lookup and update
 * cost :math:`O(\log{}n)` instead of effectively constant time.
 *
 * **Example**
 *   .. literalinclude:: ../example/sorted-map/intro.cpp
 *      :language: c++
 *      :start-after: intro/start
 *      :end-before:  intro/end
 *
 * @endrst
 *
 */
template <typename K,
          typename T,
          typename Compare       = std::less<K>,
          typename MemoryPolicy  = default_memory_policy,
          detail::bts::bits_t B  = default_bits,
          detail::bts::bits_t BL = default_bits>
class sorted_map
{
    using value_t = std::pair<K, T>;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

    struct key_fn
    {
        const K& operator()(const value_t& v) const noexcept { return v.first; }
    };

    struct project_value
    {
        const T& operator()(const value_t& v) const noexcept
        {
            return v.second;
        }
    };

    struct project_value_ptr
    {
        const T* operator()(const value_t& v) const noexcept
        {
            return &v.second;
        }
    };

    struct combine_value
    {
        template <typename Kf, typename Tf>
        value_t operator()(Kf&& k, Tf&& v) const
        {
            return {std::forward<Kf>(k), std::forward<Tf>(v)};
        }
    };

    struct default_value
    {
        const T& operator()() const
        {
            static T v{};
            return v;
        }
    };

    struct error_value
    {
        const T& operator()() const
        {
            IMMER_THROW(std::out_of_range{"key not found"});
        }
    };

    struct constantly_nullptr
    {
        const T* operator()() const { return nullptr; }
    };

    using impl_t =
        detail::bts::btree<value_t, key_fn, Compare, MemoryPolicy, B, BL>;

public:
    using key_type        = K;
    using mapped_type     = T;
    using value_type      = std::pair<K, T>;
    using size_type       = detail::bts::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare     = Compare;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator = detail::bts::
        btree_iterator<value_t, key_fn, Compare, MemoryPolicy, B, BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using transient_type =
        sorted_map_transient<K, T, Compare, MemoryPolicy, B, BL>;

    using memory_policy_type = MemoryPolicy;

    /*!
     * Constructs a map containing the elements in `values`.  If a key
     * is repeated, the last one wins.
     */
    sorted_map(std::initializer_list<value_type> values)
        : impl_{impl_t::from_initializer_list(values)}
    {
    }

    /*!
     * Constructs a map containing the elements in the range defined
     * by the input iterator `first` and range sentinel `last`.  If a
     * key is repeated, the last one wins.
     */
    template <typename Iter,
              typename Sent,
              std::enable_if_t<detail::compatible_sentinel_v<Iter, Sent>,
                               bool> = true>
    sorted_map(Iter first, Sent last)
        : impl_{impl_t::from_range(first, last)}
    {
    }

    /*!
     * Default constructor.  It creates a map of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    sorted_map() = default;

    /*!
     * Returns an iterator pointing at the element with the smallest
     * key.  It does not allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD iterator begin() const { return {impl_}; }

    /*!
     * Returns an iterator pointing just after the element with the
     * biggest key.  It does not allocate and its complexity is
     * @f$ O(1) @f$.
     */
    IMMER_NODISCARD iterator end() const
    {
        return {impl_, typename iterator::end_t{}};
    }

    /*!
     * Returns an iterator that traverses the map in reverse order,
     * starting at the element with the biggest key.
     */
    IMMER_NODISCARD reverse_iterator rbegin() const
    {
        return reverse_iterator{end()};
    }

    /*!
     * Returns an iterator just after the end of the reversed
     * collection.
     */
    IMMER_NODISCARD reverse_iterator rend() const
    {
        return reverse_iterator{begin()};
    }

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD size_type size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    IMMER_NODISCARD bool empty() const { return impl_.size == 0; }

    /*!
     * Returns a `const` reference to the entry with the smallest key.
     * Requires `!empty()`.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    const value_type& front() const
    {
        assert(!empty());
        return *begin();
    }

    /*!
     * Returns a `const` reference to the entry with the biggest key.
     * Requires `!empty()`.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    const value_type& back() const
    {
        assert(!empty());
        return *--end();
    }

    /*!
     * Returns `1` when the key `k` is contained in the map or `0`
     * otherwise. It won't allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD size_type count(const Key& k) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(k);
    }

    /*!
     * Returns `1` when the key `k` is contained in the map or `0`
     * otherwise. It won't allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD size_type count(const K& k) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, it returns a
     * default constructed value.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T& operator[](const Key& k) const
    {
        return impl_.template get<project_value, default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, it returns a
     * default constructed value.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD const T& operator[](const K& k) const
    {
        return impl_.template get<project_value, default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    const T& at(const Key& k) const
    {
        return impl_.template get<project_value, error_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    const T& at(const K& k) const
    {
        return impl_.template get<project_value, error_value>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.  If
     * the key is not contained in the map, a `nullptr` is returned.
     * It does not allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     *
     * @rst
     *
     * .. admonition:: Why doesn't this function return an iterator?
     *
     *   This function is consistent with ``immer::map``, where
     *   returning a pointer is a cheaper and more ergonomic way of
     *   telling whether the lookup succeeded.  When an iterator to
     *   the position of the key is needed, for example to traverse
     *   the neighboring entries, one can use ``lower_bound``
     *   instead.
     *
     * @endrst
     */
    IMMER_NODISCARD const T* find(const K& k) const
    {
        return impl_.template get<project_value_ptr, constantly_nullptr>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.  If
     * the key is not contained in the map, a `nullptr` is returned.
     * It does not allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T* find(const Key& k) const
    {
        return impl_.template get<project_value_ptr, constantly_nullptr>(k);
    }

    /*!
     * Returns an iterator pointing at the first entry whose key is
     * not less than `k`, or `end()` when there is none.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD iterator lower_bound(const K& k) const
    {
        return {impl_, k, typename iterator::lower_bound_t{}};
    }

    /*!
     * Returns an iterator pointing at the first entry whose key is
     * not less than `k`, or `end()` when there is none.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD iterator lower_bound(const Key& k) const
    {
        return {impl_, k, typename iterator::lower_bound_t{}};
    }

    /*!
     * Returns an iterator pointing at the first entry whose key is
     * greater than `k`, or `end()` when there is none.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD iterator upper_bound(const K& k) const
    {
        return {impl_, k, typename iterator::upper_bound_t{}};
    }

    /*!
     * Returns an iterator pointing at the first entry whose key is
     * greater than `k`, or `end()` when there is none.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD iterator upper_bound(const Key& k) const
    {
        return {impl_, k, typename iterator::upper_bound_t{}};
    }

    /*!
     * Returns whether the maps are equal.  Two maps are equal when
     * they contain equal entries in the same order, this is, keys and
     * values are compared with their `operator==`.
     */
    IMMER_NODISCARD bool operator==(const sorted_map& other) const
    {
        return impl_.root == other.impl_.root ||
               (impl_.size == other.impl_.size &&
                std::equal(begin(), end(), other.begin()));
    }
    IMMER_NODISCARD bool operator!=(const sorted_map& other) const
    {
        return !(*this == other);
    }

    /*!
     * Returns a map containing the association `value`.  If the key is
     * already in the map, it replaces its association in the map.
     * It may allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD sorted_map insert(value_type value) const&
    {
        return impl_.add(std::move(value));
    }
    IMMER_NODISCARD decltype(auto) insert(value_type value) &&
    {
        return insert_move(move_t{}, std::move(value));
    }

    /*!
     * Returns a map containing the association `(k, v)`.  If the key
     * is already in the map, it replaces its association in the map.
     * It may allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD sorted_map set(key_type k, mapped_type v) const&
    {
        return impl_.add({std::move(k), std::move(v)});
    }
    IMMER_NODISCARD decltype(auto) set(key_type k, mapped_type v) &&
    {
        return set_move(move_t{}, std::move(k), std::move(v));
    }

    /*!
     * Returns a map replacing the association `(k, v)` by the
     * association new association `(k, fn(v))`, where `v` is the
     * currently associated value for `k` in the map or a default
     * constructed value otherwise. It may allocate memory
     * and its complexity is @f$ O(\log{}n) @f$.
     */
    template <typename Fn>
    IMMER_NODISCARD sorted_map update(key_type k, Fn&& fn) const&
    {
        return impl_
            .template update<project_value, default_value, combine_value>(
                k, std::forward<Fn>(fn));
    }
    template <typename Fn>
    IMMER_NODISCARD decltype(auto) update(key_type k, Fn&& fn) &&
    {
        return update_move(move_t{}, std::move(k), std::forward<Fn>(fn));
    }

    /*!
     * Returns a map replacing the association `(k, v)` by the
     * association new association `(k, fn(v))`, where `v` is the
     * currently associated value for `k` in the map.  It does nothing
     * if `k` is not present in the map. It may allocate memory and
     * its complexity is @f$ O(\log{}n) @f$.
     */
    template <typename Fn>
    IMMER_NODISCARD sorted_map update_if_exists(key_type k, Fn&& fn) const&
    {
        return impl_.template update_if_exists<project_value, combine_value>(
            k, std::forward<Fn>(fn));
    }
    template <typename Fn>
    IMMER_NODISCARD decltype(auto) update_if_exists(key_type k, Fn&& fn) &&
    {
        return update_if_exists_move(
            move_t{}, std::move(k), std::forward<Fn>(fn));
    }

    /*!
     * Returns a map without the key `k`.  If the key is not
     * associated in the map it returns the same map.  It may allocate
     * memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD sorted_map erase(const K& k) const& { return impl_.sub(k); }
    IMMER_NODISCARD decltype(auto) erase(const K& k) &&
    {
        return erase_move(move_t{}, k);
    }

    /*!
     * Returns a @a transient form of this container, an
     * `immer::sorted_map_transient`.
     */
    IMMER_NODISCARD transient_type transient() const&
    {
        return transient_type{impl_};
    }
    IMMER_NODISCARD transient_type transient() &&
    {
        return transient_type{std::move(impl_)};
    }

    /*!
     * Returns a value that can be used as identity for the container.  If two
     * values have the same identity, they are guaranteed to be equal and to
     * contain the same objects.  However, two equal containers are not
     * guaranteed to have the same identity.
     */
    void* identity() const { return impl_.root; }

    // Semi-private
    const impl_t& impl() const { return impl_; }

private:
    friend transient_type;

    sorted_map&& insert_move(std::true_type, value_type value)
    {
        impl_.add_mut({}, std::move(value));
        return std::move(*this);
    }
    sorted_map insert_move(std::false_type, value_type value)
    {
        return impl_.add(std::move(value));
    }

    sorted_map&& set_move(std::true_type, key_type k, mapped_type v)
    {
        impl_.add_mut({}, {std::move(k), std::move(v)});
        return std::move(*this);
    }
    sorted_map set_move(std::false_type, key_type k, mapped_type v)
    {
        return impl_.add({std::move(k), std::move(v)});
    }

    template <typename Fn>
    sorted_map&& update_move(std::true_type, key_type k, Fn&& fn)
    {
        impl_.template update_mut<project_value, default_value, combine_value>(
            {}, k, std::forward<Fn>(fn));
        return std::move(*this);
    }
    template <typename Fn>
    sorted_map update_move(std::false_type, key_type k, Fn&& fn)
    {
        return impl_
            .template update<project_value, default_value, combine_value>(
                k, std::forward<Fn>(fn));
    }

    template <typename Fn>
    sorted_map&& update_if_exists_move(std::true_type, key_type k, Fn&& fn)
    {
        impl_.template update_if_exists_mut<project_value, combine_value>(
            {}, k, std::forward<Fn>(fn));
        return std::move(*this);
    }
    template <typename Fn>
    sorted_map update_if_exists_move(std::false_type, key_type k, Fn&& fn)
    {
        return impl_.template update_if_exists<project_value, combine_value>(
            k, std::forward<Fn>(fn));
    }

    sorted_map&& erase_move(std::true_type, const key_type& k)
    {
        impl_.sub_mut({}, k);
        return std::move(*this);
    }
    sorted_map erase_move(std::false_type, const key_type& k)
    {
        return impl_.sub(k);
    }

    // for immer::persist
public:
    sorted_map(impl_t impl)
        : impl_(std::move(impl))
    {
    }

private:
    impl_t impl_ = impl_t::empty();
};

static_assert(std::is_nothrow_move_constructible<sorted_map<int, int>>::value,
              "sorted_map is not nothrow move constructible");
static_assert(std::is_nothrow_move_assignable<sorted_map<int, int>>::value,
              "sorted_map is not nothrow move assignable");

} // namespace immer
