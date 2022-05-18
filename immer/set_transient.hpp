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
    using base_t = typename MemoryPolicy::transience_t::owner;

public:
    using value_type      = T;
    using size_type       = detail::hamts::size_t;
    using diference_type  = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = Equal;
    using reference       = const T&;
    using const_reference = const T&;

    using persistent_type = set<T, Hash, Equal, MemoryPolicy, B>;

    set_transient() = default;

    IMMER_NODISCARD persistent_type persistent() & { return {}; }
    IMMER_NODISCARD persistent_type persistent() && { return {}; }

private:
    friend persistent_type;
    using impl_t = typename persistent_type::impl_t;

    set_transient(impl_t impl)
        : impl_(std::move(impl))
    {}

    impl_t impl_ = impl_t::empty();
};

} // namespace immer
