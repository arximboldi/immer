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

namespace immer {

/*!
 * Immutable set of values of type `T`, sorted according to `Compare`.
 *
 * @tparam T    The type of the values to be stored in the container.
 * @tparam Compare The type of a function object capable of comparing
 *              values of type `T`.  It must induce a strict weak
 *              ordering and be stateless and default constructible.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *              memory_policy.
 *
 * @rst
 *
 * This container provides a good trade-off between cache locality,
 * search, update performance and structural sharing.  It does so by
 * storing the data in contiguous chunks of :math:`2^{BL}` elements
 * ordered by `Compare`, indexed by inner nodes of :math:`2^{B}`
 * children.  When storing big objects, the size of these contiguous
 * chunks can become too big, damaging performance.  If this is
 * measured to be problematic for a specific use-case, it can be
 * solved by using an `immer::box` to wrap the type `T`.
 *
 * In contrast to `immer::set`, iteration happens in the order defined
 * by `Compare`, and the container supports order based queries like
 * `lower_bound` and `upper_bound`.  In exchange, lookup and update
 * cost :math:`O(\log{}n)` instead of effectively constant time.
 *
 * **Example**
 *   .. literalinclude:: ../example/sorted-set/intro.cpp
 *      :language: c++
 *      :start-after: intro/start
 *      :end-before:  intro/end
 *
 * @endrst
 *
 */
template <typename T,
          typename Compare       = std::less<T>,
          typename MemoryPolicy  = default_memory_policy,
          detail::bts::bits_t B  = default_bits,
          detail::bts::bits_t BL = default_bits>
class sorted_set
{
    struct key_fn
    {
        const T& operator()(const T& v) const noexcept { return v; }
    };

    struct project_value_ptr
    {
        const T* operator()(const T& v) const noexcept { return &v; }
    };

    struct constantly_nullptr
    {
        const T* operator()() const { return nullptr; }
    };

    using impl_t = detail::bts::btree<T, key_fn, Compare, MemoryPolicy, B, BL>;

public:
    using key_type        = T;
    using value_type      = T;
    using size_type       = detail::bts::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare     = Compare;
    using reference       = const T&;
    using const_reference = const T&;

    using iterator =
        detail::bts::btree_iterator<T, key_fn, Compare, MemoryPolicy, B, BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using memory_policy_type = MemoryPolicy;

    /*!
     * Constructs a set containing the elements in `values`.
     */
    sorted_set(std::initializer_list<value_type> values)
        : impl_{impl_t::from_initializer_list(values)}
    {
    }

    /*!
     * Constructs a set containing the elements in the range defined
     * by the input iterator `first` and range sentinel `last`.
     */
    template <typename Iter,
              typename Sent,
              std::enable_if_t<detail::compatible_sentinel_v<Iter, Sent>,
                               bool> = true>
    sorted_set(Iter first, Sent last)
        : impl_{impl_t::from_range(first, last)}
    {
    }

    /*!
     * Default constructor.  It creates a set of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    sorted_set() = default;

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
     * Returns an iterator that traverses the set in reverse order,
     * starting at the biggest element.
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
     * Returns a `const` reference to the smallest element.  Requires
     * `!empty()`.  It does not allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    const T& front() const
    {
        assert(!empty());
        return *begin();
    }

    /*!
     * Returns a `const` reference to the biggest element.  Requires
     * `!empty()`.  It does not allocate memory and its complexity is
     * @f$ O(\log{}n) @f$.
     */
    const T& back() const
    {
        assert(!empty());
        return *--end();
    }

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
        return impl_.template get<project_value_ptr, constantly_nullptr>(value);
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
        return impl_.template get<project_value_ptr, constantly_nullptr>(value);
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
     * Returns an iterator pointing at the first element that is not
     * less than `value`, or `end()` when there is none.  It does not
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD iterator lower_bound(const Key& value) const
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
     * Returns an iterator pointing at the first element that is
     * greater than `value`, or `end()` when there is none.  It does
     * not allocate memory and its complexity is @f$ O(\log{}n) @f$.
     *
     * This overload participates in overload resolution only if
     * `Compare::is_transparent` is valid and denotes a type.
     */
    template <typename Key,
              typename U = Compare,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD iterator upper_bound(const Key& value) const
    {
        return {impl_, value, typename iterator::upper_bound_t{}};
    }

    /*!
     * Returns whether the sets are equal.  Two sets are equal when
     * they contain equal elements in the same order, this is,
     * elements are compared with their `operator==`.
     */
    IMMER_NODISCARD bool operator==(const sorted_set& other) const
    {
        return impl_.root == other.impl_.root ||
               (impl_.size == other.impl_.size &&
                std::equal(begin(), end(), other.begin()));
    }
    IMMER_NODISCARD bool operator!=(const sorted_set& other) const
    {
        return !(*this == other);
    }

    /*!
     * Returns a set containing `value`.  If the value is already in
     * the set, it replaces the equivalent value in the set.  It may
     * allocate memory and its complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD sorted_set insert(T value) const
    {
        return impl_.add(std::move(value));
    }

    /*!
     * Returns a set without `value`.  If the value is not in the set
     * it returns the same set.  It may allocate memory and its
     * complexity is @f$ O(\log{}n) @f$.
     */
    IMMER_NODISCARD sorted_set erase(const T& value) const
    {
        return impl_.sub(value);
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

    // for immer::persist
public:
    sorted_set(impl_t impl)
        : impl_(std::move(impl))
    {
    }

private:
    impl_t impl_ = impl_t::empty();
};

static_assert(std::is_nothrow_move_constructible<sorted_set<int>>::value,
              "sorted_set is not nothrow move constructible");
static_assert(std::is_nothrow_move_assignable<sorted_set<int>>::value,
              "sorted_set is not nothrow move assignable");

} // namespace immer
