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

namespace immer {

template <typename T,
          typename Compare,
          typename MemoryPolicy,
          detail::bts::bits_t B,
          detail::bts::bits_t BL>
class sorted_set;

/*!
 * Mutable version of `immer::sorted_set`.
 *
 * @rst
 *
 * Refer to :doc:`transients` to learn more about when and how to use
 * the mutable versions of immutable containers.
 *
 * @endrst
 */
template <typename T,
          typename Compare       = std::less<T>,
          typename MemoryPolicy  = default_memory_policy,
          detail::bts::bits_t B  = default_bits,
          detail::bts::bits_t BL = default_bits>
class sorted_set_transient : MemoryPolicy::transience_t::owner
{
    using base_t  = typename MemoryPolicy::transience_t::owner;
    using owner_t = base_t;

public:
    using persistent_type = sorted_set<T, Compare, MemoryPolicy, B, BL>;

    using key_type        = T;
    using value_type      = T;
    using size_type       = detail::bts::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare     = Compare;
    using reference       = const T&;
    using const_reference = const T&;

    using iterator =
        detail::bts::btree_iterator<T,
                                    typename persistent_type::key_fn,
                                    Compare,
                                    MemoryPolicy,
                                    B,
                                    BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    /*!
     * Default constructor.  It creates a set of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    sorted_set_transient() = default;

    /*!
     * Returns an iterator pointing at the smallest element.  It does
     * not allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD iterator begin() const { return {impl_}; }

    /*!
     * Returns an iterator pointing just after the biggest element.
     * It does not allocate and its complexity is @f$ O(1) @f$.
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
     * Returns `1` when `value` is contained in the set or `0`
     * otherwise. It won't allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD size_type count(const Key& value) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(value);
    }

    /*!
     * Returns `1` when `value` is contained in the set or `0`
     * otherwise. It won't allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD size_type count(const T& value) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(value);
    }

    /*!
     * Returns a pointer to the equivalent element in the set, or
     * `nullptr` when it is not contained in the set.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD const T* find(const T& value) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(value);
    }

    /*!
     * Returns a pointer to the equivalent element in the set, or
     * `nullptr` when it is not contained in the set.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T* find(const Key& value) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(value);
    }

    /*!
     * Returns an iterator pointing at the first element that is not
     * less than `value`, or `end()` when there is none.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD iterator lower_bound(const T& value) const
    {
        return {impl_, value, typename iterator::lower_bound_t{}};
    }

    /*!
     * Returns an iterator pointing at the first element that is
     * greater than `value`, or `end()` when there is none.  It does
     * not allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD iterator upper_bound(const T& value) const
    {
        return {impl_, value, typename iterator::upper_bound_t{}};
    }

    /*!
     * Inserts `value` into the set.  If the value is already in the
     * set, it replaces the equivalent value in the set.  It may
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    void insert(T value) { impl_.add_mut(*this, std::move(value)); }

    /*!
     * Removes `value` from the set.  Does nothing if the value is not
     * in the set.  It may allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    void erase(const T& value) { impl_.sub_mut(*this, value); }

    /*!
     * Returns an @a immutable form of this container, an
     * `immer::sorted_set`.
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

    sorted_set_transient(impl_t impl)
        : impl_(std::move(impl))
    {
    }

    impl_t impl_ = impl_t::empty();

public:
    // Semi-private
    const impl_t& impl() const { return impl_; }
};

} // namespace immer
