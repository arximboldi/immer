
#include <immu/vektor.hpp>

#include <doctest.h>
#include <boost/range/adaptors.hpp>

#include <algorithm>
#include <numeric>
#include <vector>

using namespace immu;

TEST_CASE("instantiation")
{
    auto v = vektor<int>{};
    CHECK(v.size() == 0u);
}

TEST_CASE("push back one element")
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

TEST_CASE("iterator")
{
    const auto n = 666u;
    auto v = vektor<unsigned>{};
    for (auto i = 0u; i < n; ++i)
        v = v.push_back(i);

    SUBCASE("works with range loop")
    {
        auto i = 0u;
        for (const auto& x : v)
            CHECK(x == i++);
        CHECK(i == v.size());
    }

    SUBCASE("works with standard algorithms")
    {
        auto s = std::vector<unsigned>(n);
        std::iota(s.begin(), s.end(), 0u);
        std::equal(v.begin(), v.end(), s.begin(), s.end());
    }

    SUBCASE("can go back from end")
    {
        CHECK(n-1 == *--v.end());
    }

    SUBCASE("works with reversed range adaptor")
    {
        auto r = v | boost::adaptors::reversed;
        auto i = n;
        for (const auto& x : r)
            CHECK(x == --i);
    }

    SUBCASE("works with strided range adaptor")
    {
        auto r = v | boost::adaptors::strided(5);
        auto i = 0u;
        for (const auto& x : r)
            CHECK(x == 5 * i++);
    }

    SUBCASE("works reversed")
    {
        auto i = n;
        for (auto iter = v.rbegin(), last = v.rend(); iter != last; ++iter)
            CHECK(*iter == --i);
    }

    SUBCASE("advance and distance")
    {
        auto i1 = v.begin();
        auto i2 = i1 + 100;
        CHECK(*i2 == 100u);
        CHECK(i2 - i1 == 100);
        CHECK(*(i2 - 50) == 50u);
        CHECK((i2 - 30) - i2 == -30);
    }
}
