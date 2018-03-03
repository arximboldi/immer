#include <immer/detail/type_traits.hpp>
#include <vector>
#include <catch.hpp>

TEST_CASE("is dereferenceable")
{
    SECTION("iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_dereferenceable<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::is_dereferenceable<Iter>, "");
    }

    SECTION("not dereferenceable")
    {
        static_assert(not immer::detail::is_dereferenceable<int>, "");
    }
}
