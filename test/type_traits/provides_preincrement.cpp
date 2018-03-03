#include <immer/detail/type_traits.hpp>
#include <vector>
#include <catch.hpp>

TEST_CASE("provides preincrement")
{
    SECTION("iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::provides_preincrement<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::provides_preincrement<Iter>, "");
    }

    SECTION("does not provide preincrement")
    {
        struct type{};
        static_assert(not immer::detail::provides_preincrement<type>, "");
    }
}
