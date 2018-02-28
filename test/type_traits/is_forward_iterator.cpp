#include <immer/detail/type_traits.hpp>
#include <vector>
#include <list>
#include <forward_list>
#include <catch.hpp>

TEST_CASE("is iterator")
{
    SECTION("random access iterator")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::is_forward_iterator<Iter>, "");
    }

    SECTION("bidirectional iterator")
    {
        using Iter = std::list<int>::iterator;
        static_assert(immer::detail::is_forward_iterator<Iter>, "");
    }

    SECTION("forward iterator")
    {
        using Iter = std::forward_list<int>::iterator;
        static_assert(immer::detail::is_forward_iterator<Iter>, "");
    }

    SECTION("input iterator")
    {
        using Iter = std::istream_iterator;
        static_assert(not immer::detail::is_forward_iterator<Iter>, "");
    }

    SECTION("output iterator")
    {
        using Iter = std::ostream_iterator;
        static_assert(not immer::detail::is_forward_iterator<Iter>, "");
    }

    SECTION("pointer")
    {
        using Iter = char*;
        static_assert(immer::detail::is_iterator<Iter>, "");
    }
}
