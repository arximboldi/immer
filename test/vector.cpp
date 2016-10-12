
#include <immer/vector.hpp>

#include "util.hpp"

#include <catch.hpp>
#include <boost/range/adaptors.hpp>

#include <algorithm>
#include <numeric>
#include <vector>

using namespace immer;

template <unsigned B = default_bits>
auto make_test_vector(std::size_t min, std::size_t max)
{
    auto v = vector<unsigned, B>{};
    for (auto i = min; i < max; ++i)
        v = v.push_back(i);
    return v;
}

TEST_CASE("instantiation")
{
    auto v = vector<int>{};
    CHECK(v.size() == 0u);
}

TEST_CASE("push back one element")
{
    SECTION("one element")
    {
        const auto v1 = vector<int>{};
        auto v2 = v1.push_back(42);
        CHECK(v1.size() == 0u);
        CHECK(v2.size() == 1u);
        CHECK(v2[0] == 42);
    }

    SECTION("many elements")
    {
        const auto n = 666u;
        auto v = vector<unsigned>{};
        for (auto i = 0u; i < n; ++i) {
            v = v.push_back(i * 42);
            CHECK(v.size() == i + 1);
            for (auto j = 0u; j < v.size(); ++j)
                CHECK(v[j] == j * 42);
        }
    }
}

TEST_CASE("update")
{
    const auto n = 42u;
    auto v = make_test_vector(0, n);

    SECTION("assoc")
    {
        const auto u = v.assoc(3u, 13u);
        CHECK(u.size() == v.size());
        CHECK(u[2u] == 2u);
        CHECK(u[3u] == 13u);
        CHECK(u[4u] == 4u);
        CHECK(u[40u] == 40u);
        CHECK(v[3u] == 3u);
    }

    SECTION("assoc further")
    {
        auto v = make_test_vector(0, 666);

        auto u = v.assoc(3u, 13u);
        u = u.assoc(200u, 7u);
        CHECK(u.size() == v.size());

        CHECK(u[2u] == 2u);
        CHECK(u[4u] == 4u);
        CHECK(u[40u] == 40u);
        CHECK(u[600u] == 600u);

        CHECK(u[3u] == 13u);
        CHECK(u[200u] == 7u);

        CHECK(v[3u] == 3u);
        CHECK(v[200u] == 200u);
    }

    SECTION("assoc further more")
    {
        auto v = make_test_vector<4>(0, 1000);

        for (auto i = 0u; i < v.size(); ++i) {
            v = v.assoc(i, i+1);
            CHECK(v[i] == i+1);
        }
    }

    SECTION("update")
    {
        const auto u = v.update(10u, [] (auto x) { return x + 10; });
        CHECK(u.size() == v.size());
        CHECK(u[10u] == 20u);
        CHECK(v[40u] == 40u);

        const auto w = v.update(40u, [] (auto x) { return x - 10; });
        CHECK(w.size() == v.size());
        CHECK(w[40u] == 30u);
        CHECK(v[40u] == 40u);
    }
}

TEST_CASE("iterator")
{
    const auto n = 666u;
    auto v = make_test_vector(0, n);

    SECTION("works with range loop")
    {
        auto i = 0u;
        for (const auto& x : v)
            CHECK(x == i++);
        CHECK(i == v.size());
    }

    SECTION("works with standard algorithms")
    {
        auto s = std::vector<unsigned>(n);
        std::iota(s.begin(), s.end(), 0u);
        std::equal(v.begin(), v.end(), s.begin(), s.end());
    }

    SECTION("can go back from end")
    {
        auto expected  = n - 1;
        CHECK(expected == *--v.end());
    }

    SECTION("works with reversed range adaptor")
    {
        auto r = v | boost::adaptors::reversed;
        auto i = n;
        for (const auto& x : r)
            CHECK(x == --i);
    }

    SECTION("works with strided range adaptor")
    {
        auto r = v | boost::adaptors::strided(5);
        auto i = 0u;
        for (const auto& x : r)
            CHECK(x == 5 * i++);
    }

    SECTION("works reversed")
    {
        auto i = n;
        for (auto iter = v.rbegin(), last = v.rend(); iter != last; ++iter)
            CHECK(*iter == --i);
    }

    SECTION("advance and distance")
    {
        auto i1 = v.begin();
        auto i2 = i1 + 100;
        CHECK(100u == *i2);
        CHECK(100  == i2 - i1);
        CHECK(50u  == *(i2 - 50));
        CHECK(-30  == (i2 - 30) - i2);
    }
}

TEST_CASE("reduce")
{
    const auto n = 666u;
    auto v = make_test_vector(0, n);

    SECTION("sum collection")
    {
        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = v.size() * (v.size() - 1) / 2;
        CHECK(sum == expected);
    }
}

TEST_CASE("vector of strings")
{
    // check with valgrind
    const auto n = 666u;
    auto v = vector<std::string>{};
    for (auto i = 0u; i < n; ++i)
        v = v.push_back(std::to_string(i));

    for (auto i = 0u; i < v.size(); ++i)
        CHECK(v[i] == std::to_string(i));

    SECTION("assoc")
    {
        for (auto i = 0u; i < n; ++i)
            v = v.assoc(i, "foo " + std::to_string(i));
        for (auto i = 0u; i < n; ++i)
            CHECK(v[i] == "foo " + std::to_string(i));
    }
}

struct non_default
{
    unsigned value;
    non_default() = delete;
};

TEST_CASE("non default")
{
    const auto n = 666u;
    auto v = vector<non_default>{};
    for (auto i = 0u; i < n; ++i)
        v = v.push_back({ i });

    for (auto i = 0u; i < v.size(); ++i)
        CHECK(v[i].value == i);

    SECTION("assoc")
    {
        for (auto i = 0u; i < n; ++i)
            v = v.assoc(i, {i + 1});
        for (auto i = 0u; i < n; ++i)
            CHECK(v[i].value == i + 1);
    }
}


TEST_CASE("take")
{
    const auto n = 666u;

    SECTION("anywhere")
    {
        auto v = make_test_vector<3>(0, n);

        for (auto i : test_irange(0u, n)) {
            auto vv = v.take(i);
            CHECK(vv.size() == i);
            CHECK_VECTOR_EQUALS_RANGE(vv, v.begin(), v.begin() + i);
        }
    }
}
