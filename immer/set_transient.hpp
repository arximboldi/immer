//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/hamts/champ.hpp>
#include <immer/memory_policy.hpp>

#include <functional>

namespace immer {

/*!
 * @rst
 *
 * .. admonition:: Become a sponsor!
 *    :class: danger
 *
 *    This component is planned but it has **not been implemented yet**.
 *
 *    Transiens can critically improve the performance of applications
 *    intensively using ``set`` and ``map``. If you are working for an
 *    organization using the library in a commercial project, please consider
 *    **sponsoring this work**: juanpe@sinusoid.al
 *
 * @endrst
 */
template <typename T,
          typename Hash           = std::hash<T>,
          typename Equal          = std::equal_to<T>,
          typename MemoryPolicy   = default_memory_policy,
          detail::hamts::bits_t B = default_bits>
class set_transient : MemoryPolicy::transience_t::owner
{
    using base_t  = typename MemoryPolicy::transience_t::owner;
    using owner_t = base_t;

public:
    using persistent_type = set<T, Hash, Equal, MemoryPolicy, B>;

    using value_type      = T;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const T&;
    using const_reference = const T&;

    using iterator       = typename persistent_type::iterator;
    using const_iterator = iterator;

    /*!
     * Default constructor.  It creates a set of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    set_transient() = default;

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    IMMER_NODISCARD iterator begin() const { return {impl_}; }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
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
     * *effectively* @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    template <typename K,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD size_type count(const K& value) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(value);
    }

    /*!
     * Returns `1` when `value` is contained in the set or `0`
     * otherwise. It won't allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    IMMER_NODISCARD size_type count(const T& value) const
    {
        return impl_.template get<detail::constantly<size_type, 1>,
                                  detail::constantly<size_type, 0>>(value);
    }

    /*!
     * Returns a pointer to the value if `value` is contained in the
     * set, or nullptr otherwise.
     * It does not allocate memory and its complexity is *effectively*
     * @f$ O(1) @f$.
     */
    IMMER_NODISCARD const T* find(const T& value) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(value);
    }

    /*!
     * Returns a pointer to the value if `value` is contained in the
     * set, or nullptr otherwise.
     * It does not allocate memory and its complexity is *effectively*
     * @f$ O(1) @f$.
     *
     * This overload participates in overload resolution only if
     * `Hash::is_transparent` is valid and denotes a type.
     */
    template <typename K,
              typename U = Hash,
              typename   = typename U::is_transparent>
    IMMER_NODISCARD const T* find(const K& value) const
    {
        return impl_.template get<typename persistent_type::project_value_ptr,
                                  detail::constantly<const T*, nullptr>>(value);
    }

    /*!
     * Inserts `value` into the set, and does nothing if the value is already
     * there  It may allocate memory and its complexity is *effectively* @f$
     * O(1) @f$.
     */
    void insert(T value) { impl_.add_mut(*this, std::move(value)); }

    /*!
     * Removes the `value` from the set, doing nothing if the value is not in
     * the set.  It may allocate memory and its complexity is *effectively* @f$
     * O(1) @f$.
     */
    void erase(const T& value)
    {
        // xxx: implement mutable version
        impl_ = impl_.sub(value);
    }

    /*!
     * Returns an @a immutable form of this container, an
     * `immer::set`.
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

    set_transient(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();

public:
    // Semi-private
    const impl_t& impl() const { return impl_; }
};

} // namespace immer
