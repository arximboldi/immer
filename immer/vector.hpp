//
// immer - immutable data structures for C++
// Copyright (C) 2016 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <immer/detail/rbts/rbtree.hpp>
#include <immer/detail/rbts/rbtree_iterator.hpp>
#include <immer/memory_policy.hpp>

#if IMMER_DEBUG_PRINT
#include <immer/flex_vector.hpp>
#endif

namespace immer {

template <typename T,
          typename MemoryPolicy,
          detail::rbts::bits_t B,
          detail::rbts::bits_t BL>
class flex_vector;

/**
 * Immutable sequential container supporting both random access and
 * structural sharing.
 *
 * @tparam T The type of the values to be stored in the container.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *         memory_policy.
 *
 * @rst
 *
 * This cotainer provides a good trade of between cache locality,
 * random access, update performance and structural sharing.  It does
 * so by storing the data in contiguous chunks of :math:`2^{BL}`
 * elements.  By default, when ``sizeof(T) == sizeof(void*)`` then
 * :math:`B=BL=5`, such that data would be stored in contiguous
 * chunks of :math:`32` elements.
 *
 * You may learn more about the meaning and implications of ``B`` and
 * ``BL`` parameters in the :doc:`implementation` section.
 *
 * .. note:: In several methods we say that their complexity is
 *    *effectively* :math:`O(...)`. Do not confuse this with the word
 *    *amortized*, which has a very different meaning.  In this
 *    context, *effective* means that while the
 *    mathematically rigurous
 *    complexity might be higher, for all practical matters the
 *    provided complexity is more useful to think about the actual
 *    cost of the operation.
 *
 * **Example**
 *   .. literalinclude:: ../example/vector/intro.cpp
 *      :language: c++
 *
 * @endrst
 */
template <typename T,
          typename MemoryPolicy   = default_memory_policy,
          detail::rbts::bits_t B  = default_bits,
          detail::rbts::bits_t BL = detail::rbts::derive_bits_leaf<T, MemoryPolicy, B>>
class vector
{
    using impl_t = detail::rbts::rbtree<T, MemoryPolicy, B, BL>;

public:
    static constexpr auto bits = B;
    static constexpr auto bits_leaf = BL;
    using memory_policy = MemoryPolicy;

    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::rbts::rbtree_iterator<T, MemoryPolicy, B, BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    /*!
     * Default constructor.  It creates a vector of `size() == 0`.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    vector() = default;

    /*!
     * Returns an iterator pointing at the first element of the
     * collection. It does not allocate memory and its complexity is
     * @f$ O(1) @f$.
     */
    iterator begin() const { return {impl_}; }

    /*!
     * Returns an iterator pointing just after the last element of the
     * collection. It does not allocate and its complexity is @f$ O(1) @f$.
     */
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing at the first element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rbegin() const { return reverse_iterator{end()}; }

    /*!
     * Returns an iterator that traverses the collection backwards,
     * pointing after the last element of the reversed collection. It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    /*!
     * Returns the number of elements in the container.  It does
     * not allocate memory and its complexity is @f$ O(1) @f$.
     */
    std::size_t size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    bool empty() const { return impl_.size == 0; }

    /*!
     * Returns a `const` reference to the element at position `index`.
     * It does not allocate memory and its complexity is *effectively*
     * @f$ O(1) @f$.
     */
    reference operator[] (size_type index) const
    { return impl_.get(index); }

    /*!
     * Returns a vector with `value` inserted at the end.  It may
     * allocate memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    vector push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    /*!
     * Returns a vector containing value `value` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    vector assoc(std::size_t index, value_type value) const
    { return { impl_.assoc(index, std::move(value)) }; }

    /*!
     * Returns a vector containing the result of the expression
     * `fn((*this)[idx])` at position `idx`.
     * Undefined for `0 >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     */
    template <typename FnT>
    vector update(std::size_t index, FnT&& fn) const
    { return { impl_.update(index, std::forward<FnT>(fn)) }; }

    /*!
     * Returns a vector containing `min(elems, size())` elements. It may
     * allocate memory and its complexity is *effectively* @f$ O(1) @f$.
     */
    vector take(std::size_t elems) const
    { return { impl_.take(elems) }; }

    /*!
     * Fold the vector.
     */
    template <typename Step, typename State>
    State reduce(Step&& step, State&& init) const
    { return impl_.reduce(std::forward<Step>(step),
                          std::forward<State>(init)); }

#if IMMER_DEBUG_PRINT
    void debug_print() const
    { flex_vector<T, MemoryPolicy, B, BL>{*this}.debug_print(); }
#endif

private:
    friend class flex_vector<T, MemoryPolicy, B, BL>;

    vector(impl_t impl)
        : impl_(std::move(impl))
    {
#if IMMER_DEBUG_PRINT
        // force the compiler to generate debug_print, so we can call
        // it from a debugger
        [](volatile auto){}(&vector::debug_print);
#endif
    }

    impl_t impl_ = impl_t::empty;
};

} // namespace immer
