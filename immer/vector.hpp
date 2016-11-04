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

    vector() = default;

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    vector push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    vector assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    vector update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

    vector take(std::size_t elems) const
    { return { impl_.take(elems) }; }

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
