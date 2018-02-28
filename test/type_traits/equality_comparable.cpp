#include <immer/detail/type_traits.hpp>
#include <vector>
#include <catch.hpp>

struct string_sentinel {};
bool operator==(const char* i, string_sentinel);
bool operator!=(const char* i, string_sentinel);

TEST_CASE("equality comparable")
{
    SECTION("iterator pairs")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::equality_comparable<Iter, Iter>, "");
    }

    SECTION("pointer pairs")
    {
        using Iter = char*;
        static_assert(immer::detail::equality_comparable<Iter, Iter>, "");
    }

    SECTION("iterator/sentinel pair")
    {
        using Iter = char*;
        using Sent = string_sentinel;
        static_assert(immer::detail::equality_comparable<Iter, Sent>, "");
    }
}
