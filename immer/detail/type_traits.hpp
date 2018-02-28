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

#include <type_traits>
#include <iterator>
#include <utility>

namespace immer {
namespace detail {

template <typename... Ts>
struct make_void { using type = void; };

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename T, typename = void>
struct is_dereferenceable_t : std::false_type {};

template <typename T>
struct is_dereferenceable_t<T, void_t<decltype(*(std::declval<T&>()))>> :
    std::true_type {};

template <typename T>
constexpr bool is_dereferenceable = is_dereferenceable_t<T>::value;

template <typename T, typename = void>
struct provides_preincrement_t : std::false_type {};

template <typename T>
struct provides_preincrement_t
<T, std::enable_if_t
 <std::is_same<T&, decltype(++(std::declval<T&>()))>::value>> :
    std::true_type {};

template <typename T>
constexpr bool provides_preincrement = provides_preincrement_t<T>::value;

using std::swap;

template <typename T, typename U, typename = void>
struct is_swappable_with_t : std::false_type {};

// Does not account for non-referenceable types
template <typename T, typename U>
struct is_swappable_with_t
<T, U, void_t<decltype(swap(std::declval<T&>(), std::declval<U&>())),
              decltype(swap(std::declval<U&>(), std::declval<T&>()))>> :
    std::true_type {};

template <typename T>
constexpr bool is_swappable = is_swappable_with_t<T&, T&>::value;

template <typename T, typename = void>
struct is_iterator_t : std::false_type {};

// See http://en.cppreference.com/w/cpp/concept/Iterator
template <typename T>
struct is_iterator_t
<T, void_t
 <std::enable_if_t
  <provides_preincrement<T>
   and is_dereferenceable<T>
   // accounts for non-referenceable types
   and std::is_copy_constructible<T>::value
   and std::is_copy_assignable<T>::value
   and std::is_destructible<T>::value
   and is_swappable<T>>,
  typename std::iterator_traits<T>::value_type,
  typename std::iterator_traits<T>::difference_type,
  typename std::iterator_traits<T>::reference,
  typename std::iterator_traits<T>::pointer,
  typename std::iterator_traits<T>::iterator_category>> :
    std::true_type {};

template<typename T>
constexpr bool is_iterator = is_iterator_t<T>::value;

template<typename T, typename U=T, typename = void>
struct equality_comparable_t : std::false_type {};

template<typename T, typename U>
struct equality_comparable_t
<T, U, std::enable_if_t
 <std::is_same
  <bool, decltype(std::declval<T&>() == std::declval<U&>())>::value>> :
    std::true_type {};

template<typename T, typename U=T>
constexpr bool equality_comparable = equality_comparable_t<T, U>::value;

template<typename T, typename U=T, typename = void>
struct inequality_comparable_t : std::false_type {};

template<typename T, typename U>
struct inequality_comparable_t
<T, U, std::enable_if_t
 <std::is_same
  <bool, decltype(std::declval<T&>() == std::declval<U&>())>::value>> :
    std::true_type {};

template<typename T, typename U=T>
constexpr bool inequality_comparable =
  inequality_comparable_t<T, U>::value;

template<typename T, typename U, typename = void>
struct compatible_sentinel_t : std::false_type {};

template<typename T, typename U>
struct compatible_sentinel_t
<T, U, std::enable_if_t
 <is_iterator<T>
  and equality_comparable<T, U>
  and inequality_comparable<T, U>>> :
    std::true_type {};

template<typename T, typename U>
constexpr bool compatible_sentinel = compatible_sentinel_t<T,U>::value;

template<typename T, typename = void>
struct is_forward_iterator_t : std::false_type {};

template<typename T>
struct is_forward_iterator_t
<T, std::enable_if_t
 <is_iterator<T> and
  (std::is_same
   <std::forward_iterator_tag,
    typename std::iterator_traits<T>::iterator_category>::value
   or std::is_base_of
      <std::forward_iterator_tag,
       typename std::iterator_traits<T>::iterator_category>::value)>> :
    std::true_type {};

template<typename T>
constexpr bool is_forward_iterator = is_forward_iterator_t<T>::value;

}
}
