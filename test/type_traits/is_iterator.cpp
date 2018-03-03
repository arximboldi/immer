#include <immer/detail/type_traits.hpp>
#include <vector>
#include <catch.hpp>

TEST_CASE("is iterator")
{
    SECTION("iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_iterator<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::is_iterator<Iter>, "");
    }

    SECTION("not iterator")
    {
        static_assert(not immer::detail::is_iterator<int>, "");
    }
}
