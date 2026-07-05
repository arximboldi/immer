//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/bts/btree.hpp>
#include <immer/detail/bts/btree_iterator.hpp>
#include <immer/memory_policy.hpp>

#include <functional>
#include <iterator>
#include <utility>

namespace immer {

template <typename K,
          typename T,
          typename Compare,
          typename MemoryPolicy,
          detail::bts::bits_t B,
          detail::bts::bits_t BL>
class sorted_map;

/*!
 * Mutable version of `immer::sorted_map`.
 *
 * @rst
 *
 * Refer to :doc:`transients` to learn more about when and how to use
 * the mutable versions of immutable containers.
 *
 * @endrst
 */
template <typename K,
          typename T,
          typename Compare       = std::less<K>,
          typename MemoryPolicy  = default_memory_policy,
          detail::bts::bits_t B  = default_bits,
          detail::bts::bits_t BL = default_bits>
class sorted_map_transient : MemoryPolicy::transience_t::owner
{
    using base_t  = typename MemoryPolicy::transience_t::owner;
    using owner_t = base_t;

public:
    using persistent_type = sorted_map<K, T, Compare, MemoryPolicy, B, BL>;

    using key_type        = K;
    using mapped_type     = T;
    using value_type      = std::pair<K, T>;
    using size_type       = detail::bts::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare     = Compare;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator =
        detail::bts::btree_iterator<value_type,
                                    typename persistent_type::key_fn,
                                    Compare,
                                    MemoryPolicy,
                                    B,
                                    BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    /*!
     * Default constructor.  It creates a map of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    sorted_map_transient() = default;

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
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::default_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, it returns a
     * default constructed value.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD const T& operator[](const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::default_value>(k);
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
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    /*!
     * Returns a `const` reference to the values associated to the key
     * `k`.  If the key is not contained in the map, throws an
     * `std::out_of_range` error.  It does not allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    const T& at(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value,
                                  typename persistent_type::error_value>(k);
    }

    /*!
     * Returns a pointer to the value associated with the key `k`.  If
     * the key is not contained in the map, a `nullptr` is returned.
     * It does not allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD const T* find(const K& k) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
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
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(k);
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
     * greater than `k`, or `end()` when there is none.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD iterator upper_bound(const K& k) const
    {
        return {impl_, k, typename iterator::upper_bound_t{}};
    }

    /*!
     * Inserts the association `value`.  If the key is already in the
     * map, it replaces its association in the map.  It may allocate
     * memory and its complexity is @f$ O(\log{}n) @f$.
     */
    void insert(value_type value) { impl_.add_mut(*this, std::move(value)); }

    /*!
     * Inserts the association `(k, v)`.  If the key is already in the
     * map, it replaces its association in the map.  It may allocate
     * memory and its complexity is @f$ O(\log{}n) @f$.
     */
    void set(key_type k, mapped_type v)
    {
        impl_.add_mut(*this, {std::move(k), std::move(v)});
    }

    /*!
     * Replaces the association `(k, v)` by the association new
     * association `(k, fn(v))`, where `v` is the currently associated
     * value for `k` in the map or a default constructed value
     * otherwise. It may allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    template <typename Fn>
    void update(key_type k, Fn&& fn)
    {
        impl_.template update_mut<typename persistent_type::project_value,
                                  typename persistent_type::default_value,
                                  typename persistent_type::combine_value>(
            *this, k, std::forward<Fn>(fn));
    }

    /*!
     * Replaces the association `(k, v)` by the association new
     * association `(k, fn(v))`, where `v` is the currently associated
     * value for `k` in the map or does nothing if `k` is not present
     * in the map. It may allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    template <typename Fn>
    void update_if_exists(key_type k, Fn&& fn)
    {
        impl_.template update_if_exists_mut<
            typename persistent_type::project_value,
            typename persistent_type::combine_value>(
            *this, k, std::forward<Fn>(fn));
    }

    /*!
     * Removes the key `k` from the map.  Does nothing if the key is
     * not associated in the map.  It may allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    void erase(const K& k) { impl_.sub_mut(*this, k); }

    /*!
     * Returns an @a immutable form of this container, an
     * `immer::sorted_map`.
     */
    IMMER_NODISCARD persistent_type persistent() &
    {
        this->owner_t::operator=(owner_t{});
        return impl_;
    }
    IMMER_NODISCARD persistent_type persistent() && { return std::move(impl_); }

private:
    friend persistent_type;
    using impl_t = typename persistent_type::impl_t;

    sorted_map_transient(impl_t impl)
        : impl_(std::move(impl))
    {
    }

    impl_t impl_ = impl_t::empty();

public:
    // Semi-private
    const impl_t& impl() const { return impl_; }
};

} // namespace immer
