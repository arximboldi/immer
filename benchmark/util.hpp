
#pragma once

#include <utility>
#include <cstddef>
#include <limits>

namespace immer {
template <typename T> struct array;
}

namespace {

struct push_back_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    {
        return std::forward<T>(v)
            .push_back(std::forward<U>(x));
    }
};

struct push_front_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    {
        return std::forward<T>(v)
            .push_front(std::forward<U>(x));
    }
};

template <typename T>
struct get_limit : std::integral_constant<
    std::size_t, std::numeric_limits<std::size_t>::max()> {};

template <typename T>
struct get_limit<immer::array<T>> : std::integral_constant<
    std::size_t, 10000> {};

} // anonymous namespace
