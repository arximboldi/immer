#include <immer/detail/type_traits.hpp>
#include <vector>
#include <catch.hpp>

struct string_sentinel {};
bool operator==(const char* i, string_sentinel);
bool operator!=(const char* i, string_sentinel);

TEST_CASE("compatible_sentinel")
{
    SECTION("iterator pairs")
    {
        using Iter = std::vector<int>::iterator;
        static_assert(immer::detail::compatible_sentinel<Iter, Iter>, "");
    }

    SECTION("pointer pairs")
    {
        using Iter = char*;
        static_assert(immer::detail::compatible_sentinel<Iter, Iter>, "");
    }

    SECTION("iterator/sentinel pair")
    {
        using Iter = char*;
        using Sent = string_sentinel;
        static_assert(immer::detail::compatible_sentinel<Iter, Sent>, "");
    }

    SECTION("incompatible pair")
    {
        using Iter1 = std::vector<int>::iterator;
        using Iter2 = std::vector<double>::iterator;
        static_assert(not immer::detail::compatible_sentinel<Iter1, Iter2>, "");
    }
}
