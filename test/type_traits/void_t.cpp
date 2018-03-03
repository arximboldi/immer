#include <immer/detail/type_traits.hpp>
#include <catch.hpp>

TEST_CASE("void_t")
{
    static_assert(std::is_same<void, immer::detail::void_t<int>>::value, "");
}
