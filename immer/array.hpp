//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <immer/detail/array_impl.hpp>

namespace immer {

/*!
 * Immutable container that stores a sequence of elements in
 * contiguous memory.
 *
 * @tparam T The type of the values to be stored in the container.
 *
 * @rst
 *
 * It supports the most efficient iteration and random access,
 * equivalent to a ``std::vector`` or ``std::array``, but all
 * manipulations are :math:`O(size)`.
 *
 * .. tip:: Don't be fooled by the bad complexity of this data
 *    structure.  It is a great choice for short sequence or when it
 *    is seldom or never changed.  This depends on the ``sizeof(T)``
 *    and the expensiveness of its ``T``'s copy constructor, in case
 *    of doubt, measure.  For basic types, using an `array` when
 *    :math:`n < 100` is a good heuristic.
 *
 * .. warning:: The current implementation depends on
 *    ``boost::intrusive_ptr`` and does not support :doc:`memory
 *    policies<memory>`.  This will be fixed soon.
 *
 * @endrst
 */
template <typename T, typename MemoryPolicy = default_memory_policy>
class array
{
    using impl_t = detail::array_impl<T, MemoryPolicy>;

public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = typename std::vector<T>::const_iterator;
    using const_iterator   = iterator;
    using reverse_iterator = typename std::vector<T>::const_reverse_iterator;

    using memory_policy = MemoryPolicy;

    /*!
     * Default constructor.  It creates an array of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    array() = default;

    /*!
     * Constructs a vector containing the elements in `values`.
     */
    array(std::initializer_list<T> values)
        : impl_{impl_t::from_initializer_list(values)}
    {}

    /*!
     * Constructs a vector containing the elements in the range
     * defined by the input iterators `first` and `last`.
     */
    template <typename Iter>
    array(Iter first, Iter last)
        : impl_{impl_t::from_range(first, last)}
    {}

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    iterator begin() const { return impl_.d->begin(); }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
     */
    iterator end()   const { return impl_.d->end(); }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing at the first element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rbegin() const { return impl_.d->rbegin(); }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing after the last element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rend()   const { return impl_.d->rend(); }

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    std::size_t size() const { return impl_.d->size(); }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    bool empty() const { return impl_.d->empty(); }

    /*!
     * Returns a `const` reference to the element at position `index`.
     * It does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reference operator[] (size_type index) const
    { return impl_.get(index); }

    /*!
     * Returns whether the vectors are equal.
     */
    bool operator==(const array& other) const
    { return impl_.equals(other.impl_); }
    bool operator!=(const array& other) const
    { return !(*this == other); }

    /*!
     * Returns an array with `value` inserted at the end.  It may
     * allocate memory and its complexity is @f$ O(size) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/array/array.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: push-back/start
     *      :end-before:  push-back/end
     *
     * @endrst
     */
    array push_back(value_type value) const
    { return impl_.push_back(std::move(value)); }

    /*!
     * Returns an array containing value `value` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is @f$ O(size) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/array/array.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: set/start
     *      :end-before:  set/end
     *
     * @endrst
     */
    array set(std::size_t index, value_type value) const
    { return impl_.assoc(index, std::move(value)); }

    /*!
     * Returns an array containing the result of the expression
     * `fn((*this)[idx])` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is @f$ O(size) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/array/array.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: update/start
     *      :end-before:  update/end
     *
     * @endrst
     */
    template <typename FnT>
    array update(std::size_t index, FnT&& fn) const
    { return impl_.update(index, std::forward<FnT>(fn)); }

private:
    array(impl_t impl) : impl_(std::move(impl)) {}
    impl_t impl_;
};

} /* namespace immer */
