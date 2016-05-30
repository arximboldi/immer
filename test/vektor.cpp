
#include <immu/vektor.hpp>
#include <doctest.h>

using namespace immu;

TEST_CASE("instantiation")
{
    auto v = vektor<int>{};
    CHECK(v.size() == 0u);
}

TEST_CASE("push back")
{
    SUBCASE("one element")
    {
        const auto v1 = vektor<int>{};
        auto v2 = v1.push_back(42);
        CHECK(v1.size() == 0u);
        CHECK(v2.size() == 1u);
        CHECK(v2[0] == 42);
    }

    SUBCASE("many elements")
    {
        const auto n = 666u;
        auto v = vektor<unsigned>{};
        for (auto i = 0u; i < n; ++i) {
            v = v.push_back(i * 42);
            CHECK(v.size() == i + 1);
            for (auto j = 0u; j < v.size(); ++j)
                CHECK(v[j] == j * 42);
        }
    }
}
