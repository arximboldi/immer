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

#include <immer/detail/immutable.hpp>
#include <immer/detail/rbts/rrbtree.hpp>
#include <immer/detail/rbts/rrbtree_iterator.hpp>
#include <immer/memory_policy.hpp>

namespace immer {

template <typename T,
          typename MP,
          detail::rbts::bits_t B,
          detail::rbts::bits_t BL>
class vector;

template <typename T,
          typename MP,
          detail::rbts::bits_t B,
          detail::rbts::bits_t BL>
class flex_vector_transient;

/*!
 * Immutable sequential container supporting both random access,
 * structural sharing and efficient concatenation and slicing.
 *
 * @tparam T The type of the values to be stored in the container.
 * @tparam MemoryPolicy Memory management policy. See @ref
 *         memory_policy.
 *
 * @rst
 *
 * This container is very similar to `vector`_ but also supports
 * :math:`O(log(size))` *concatenation*, *slicing* and *insertion* at
 * any point. Its performance characteristics are almost identical
 * until one of these operations is performed.  After that,
 * performance is degraded by a constant factor that usually oscilates
 * in the range :math:`[1, 2)` depending on the operation and the
 * amount of flexible operations that have been performed.
 *
 * .. tip:: A `vector`_ can be converted to a `flex_vector`_ in
 *    constant time without any allocation.  This is so because the
 *    internal structure of a *vector* is a strict subset of the
 *    internal structure of a *flexible vector*.  You can take
 *    advantage of this property by creating normal vectors as long as
 *    the flexible operations are not needed, and convert later in
 *    your processing pipeline once and if these are needed.
 *
 * @endrst
 */
template <typename T,
          typename MemoryPolicy   = default_memory_policy,
          detail::rbts::bits_t B  = default_bits,
          detail::rbts::bits_t BL = detail::rbts::derive_bits_leaf<T, MemoryPolicy, B>>
class flex_vector
{
    using impl_t = detail::rbts::rrbtree<T, MemoryPolicy, B, BL>;

    using move_t =
        std::integral_constant<bool, MemoryPolicy::use_transient_rvalues>;

public:
    static constexpr auto bits = B;
    static constexpr auto bits_leaf = BL;
    using memory_policy = MemoryPolicy;

    using value_type = T;
    using reference = const T&;
    using size_type = detail::rbts::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::rbts::rrbtree_iterator<T, MemoryPolicy, B, BL>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using transient_type   = flex_vector_transient<T, MemoryPolicy, B, BL>;

    /*!
     * Default constructor.  It creates a flex_vector of `size() == 0`.
     * It does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    flex_vector() = default;

    /*!
     * Constructs a vector containing the elements in `values`.
     */
    flex_vector(std::initializer_list<T> values)
        : impl_{impl_t::from_initializer_list(values)}
    {}

    /*!
     * Constructs a vector containing the elements in the range
     * defined by the input iterators `first` and `last`.
     */
    template <typename Iter>
    flex_vector(Iter first, Iter last)
        : impl_{impl_t::from_range(first, last)}
    {}

    /*!
     * Default constructor.  It creates a flex_vector with the same
     * contents as `v`.  It does not allocate memory and is
     * @f$ O(1) @f$.
     */
    flex_vector(vector<T, MemoryPolicy, B, BL> v)
        : impl_ { v.impl_.size, v.impl_.shift,
                  v.impl_.root->inc(), v.impl_.tail->inc() }
    {}

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
    size_type size() const { return impl_.size; }

    /*!
     * Returns `true` if there are no elements in the container.  It
     * does not allocate memory and its complexity is @f$ O(1) @f$.
     */
    bool empty() const { return impl_.size == 0; }

    /*!
     * Access the last element.
     */
    const T& back() const { return impl_.back(); }

    /*!
     * Access the first element.
     */
    const T& front() const { return impl_.front(); }

    /*!
     * Returns a `const` reference to the element at position `index`.
     * Undefined for `index >= size()`.
     * It does not allocate memory and its complexity
     * is *effectively* @f$ O(1) @f$.
     */
    reference operator[] (size_type index) const
    { return impl_.get(index); }

    /*!
     * Returns whether the vectors are equal.
     */
    bool operator==(const flex_vector& other) const
    { return impl_.equals(other.impl_); }
    bool operator!=(const flex_vector& other) const
    { return !(*this == other); }

    /*!
     * Returns a flex_vector with `value` inserted at the end.  It may
     * allocate memory and its complexity is *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: push-back/start
     *      :end-before:  push-back/end
     *
     * @endrst
     */
    flex_vector push_back(value_type value) const&
    { return impl_.push_back(std::move(value)); }

    decltype(auto) push_back(value_type value) &&
    { return push_back_move(move_t{}, std::move(value)); }

    /*!
     * Returns a flex_vector with `value` inserted at the frony.  It may
     * allocate memory and its complexity is @f$ O(log(size)) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: push-front/start
     *      :end-before:  push-front/end
     *
     * @endrst
     */
    flex_vector push_front(value_type value) const
    { return flex_vector{}.push_back(value) + *this; }

    /*!
     * Returns a flex_vector containing value `value` at position `index`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: set/start
     *      :end-before:  set/end
     *
     * @endrst
     */
    flex_vector set(size_type index, value_type value) const&
    { return impl_.assoc(index, std::move(value)); }

    decltype(auto) set(size_type index, value_type value) &&
    { return set_move(move_t{}, index, std::move(value)); }

    /*!
     * Returns a vector containing the result of the expression
     * `fn((*this)[idx])` at position `idx`.
     * Undefined for `index >= size()`.
     * It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: update/start
     *      :end-before:  update/end
     *
     * @endrst

     */
    template <typename FnT>
    flex_vector update(size_type index, FnT&& fn) const&
    { return impl_.update(index, std::forward<FnT>(fn)); }

    template <typename FnT>
    decltype(auto) update(size_type index, FnT&& fn) &&
    { return update_move(move_t{}, index, std::forward<FnT>(fn)); }

    template <typename ... Rest>
    auto setIn(size_type index, Rest const & ... rest) const
    { return detail::setIn(*this, index, rest ...); }

    template <typename ... Rest>
    auto updateIn(size_type index, Rest const & ... rest) const
    { return detail::updateIn(*this, index, rest ...); }

    /*!
     * Returns a vector containing only the first `min(elems, size())`
     * elements. It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: take/start
     *      :end-before:  take/end
     *
     * @endrst
     */
    flex_vector take(size_type elems) const&
    { return impl_.take(elems); }

    decltype(auto) take(size_type elems) &&
    { return take_move(move_t{}, elems); }

    /*!
     * Returns a vector without the first `min(elems, size())`
     * elements. It may allocate memory and its complexity is
     * *effectively* @f$ O(1) @f$.
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: drop/start
     *      :end-before:  drop/end
     *
     * @endrst
     */
    flex_vector drop(size_type elems) const&
    { return impl_.drop(elems); }

    decltype(auto) drop(size_type elems) &&
    { return drop_move(move_t{}, elems); }

    /*!
     * Concatenation operator. Returns a flex_vector with the contents
     * of `l` followed by those of `r`.  It may allocate memory
     * and its complexity is @f$ O(log(max(size_r, size_l))) @f$
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: concat/start
     *      :end-before:  concat/end
     *
     * @endrst
     */
    friend flex_vector operator+ (const flex_vector& l, const flex_vector& r)
    { return l.impl_.concat(r.impl_); }

    friend decltype(auto) operator+ (flex_vector&& l, const flex_vector& r)
    { return concat_move(move_t{}, std::move(l), r); }

    friend decltype(auto) operator+ (const flex_vector& l, flex_vector&& r)
    { return concat_move(move_t{}, l, std::move(r)); }

    friend decltype(auto) operator+ (flex_vector&& l, flex_vector&& r)
    { return concat_move(move_t{}, std::move(l), std::move(r)); }

    /*!
     * Returns a flex_vector with the `value` inserted at index
     * `pos`. It may allocate memory and its complexity is @f$
     * O(log(size)) @f$
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: insert/start
     *      :end-before:  insert/end
     *
     * @endrst
     */
    flex_vector insert(size_type pos, T value) const&
    { return take(pos).push_back(std::move(value)) + drop(pos); }
    decltype(auto) insert(size_type pos, T value) &&
    {
        using std::move;
        auto rs = drop(pos);
        return std::move(*this).take(pos).push_back(
            std::move(value)) + std::move(rs);
    }

    /*!
     * Returns a flex_vector without the element at index `pos`. It
     * may allocate memory and its complexity is @f$ O(log(size)) @f$
     *
     * @rst
     *
     * **Example**
     *   .. literalinclude:: ../example/flex-vector/flex-vector.cpp
     *      :language: c++
     *      :dedent: 8
     *      :start-after: erase/start
     *      :end-before:  erase/end
     *
     * @endrst
     */
    flex_vector erase(size_type pos) const&
    { return take(pos) + drop(pos + 1); }
    decltype(auto) erase(size_type pos) &&
    {
        auto rs = drop(pos + 1);
        return std::move(*this).take(pos) + std::move(rs);
    }

    /*!
     * Returns an @a transient form of this container, an
     * `immer::flex_vector_transient`.
     */
    transient_type transient() const&
    { return transient_type{ impl_ }; }
    transient_type transient() &&
    { return transient_type{ std::move(impl_) }; }

    // Semi-private
    const impl_t& impl() const { return impl_; }

#if IMMER_DEBUG_PRINT
    void debug_print() const
    { impl_.debug_print(); }
#endif

private:
    friend transient_type;

    flex_vector(impl_t impl)
        : impl_(std::move(impl))
    {
#if IMMER_DEBUG_PRINT
        // force the compiler to generate debug_print, so we can call
        // it from a debugger
        [](volatile auto){}(&flex_vector::debug_print);
#endif
    }

    flex_vector&& push_back_move(std::true_type, value_type value)
    { impl_.push_back_mut({}, std::move(value)); return std::move(*this); }
    flex_vector push_back_move(std::false_type, value_type value)
    { return impl_.push_back(std::move(value)); }

    flex_vector&& set_move(std::true_type, size_type index, value_type value)
    { impl_.assoc_mut({}, index, std::move(value)); return std::move(*this); }
    flex_vector set_move(std::false_type, size_type index, value_type value)
    { return impl_.assoc(index, std::move(value)); }

    template <typename Fn>
    flex_vector&& update_move(std::true_type, size_type index, Fn&& fn)
    { impl_.update_mut({}, index, std::forward<Fn>(fn)); return std::move(*this); }
    template <typename Fn>
    flex_vector update_move(std::false_type, size_type index, Fn&& fn)
    { return impl_.assoc(index, std::forward<Fn>(fn)); }

    flex_vector&& take_move(std::true_type, size_type elems)
    { impl_.take_mut({}, elems); return std::move(*this); }
    flex_vector take_move(std::false_type, size_type elems)
    { return impl_.take(elems); }

    flex_vector&& drop_move(std::true_type, size_type elems)
    { impl_.drop_mut({}, elems); return std::move(*this); }
    flex_vector drop_move(std::false_type, size_type elems)
    { return impl_.drop(elems); }

    static flex_vector&& concat_move(std::true_type, flex_vector&& l, const flex_vector& r)
    { concat_mut_l(l.impl_, {}, r.impl_); return std::move(l); }
    static flex_vector&& concat_move(std::true_type, const flex_vector& l, flex_vector&& r)
    { concat_mut_r(l.impl_, r.impl_, {}); return std::move(r); }
    static flex_vector&& concat_move(std::true_type, flex_vector&& l, flex_vector&& r)
    { concat_mut_lr_l(l.impl_, {}, r.impl_, {}); return std::move(l); }
    static flex_vector concat_move(std::false_type, const flex_vector& l, const flex_vector& r)
    { return l.impl_.concat(r.impl_); }

    impl_t impl_ = impl_t::empty;
};

} // namespace immer
