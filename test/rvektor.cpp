
#include <immu/rvektor.hpp>

#include <catch.hpp>
#include <boost/range/adaptors.hpp>

#include <algorithm>
#include <numeric>
#include <vector>

using namespace immu;

namespace {

template <unsigned B=5>
auto make_test_rvektor(std::size_t min, std::size_t max)
{
    auto v = rvektor<unsigned, B>{};
    for (auto i = min; i < max; ++i)
        v = v.push_back(i);
    return v;
}

template <unsigned B=5>
auto make_test_rvektor_front(std::size_t min, std::size_t max)
{
    auto v = rvektor<unsigned, B>{};
    for (auto i = max; i > min;)
        v = v.push_front(--i);
    return v;
}

template <unsigned B=5>
auto make_many_test_rvektor(std::size_t n)
{
    using vektor_t = immu::rvektor<unsigned, B>;
    auto many = std::vector<vektor_t>{};
    many.reserve(n);
    std::generate_n(std::back_inserter(many), n,
                    [v = vektor_t{}, i = 0u] () mutable
                    {
                        auto r = v;
                        v = v.push_back(i++);
                        return r;
                    });
    return many;
}

template <unsigned B=5>
auto make_many_test_rvektor_front(std::size_t n)
{
    using vektor_t = immu::rvektor<unsigned, B>;
    auto many = std::vector<vektor_t>{};
    many.reserve(n);
    std::generate_n(std::back_inserter(many), n,
                    [i = 0u] () mutable
                    {
                        return make_test_rvektor_front<B>(0, i++);
                    });
    return many;
}

template <unsigned B=5>
auto make_many_test_rvektor_front_remainder(std::size_t n)
{
    using vektor_t = immu::rvektor<unsigned, B>;
    auto many = std::vector<vektor_t>{};
    many.reserve(n);
    std::generate_n(std::back_inserter(many), n,
                    [v = vektor_t{}, i = n-1] () mutable
                    {
                        auto r = v;
                        v = v.push_front(--i);
                        return r;
                    });
    return many;
}

} // anonymous namespace

TEST_CASE("instantiation")
{
    auto v = rvektor<int>{};
    CHECK(v.size() == 0u);
}

TEST_CASE("push_back")
{
    SECTION("one element")
    {
        const auto v1 = rvektor<int>{};
        auto v2 = v1.push_back(42);
        CHECK(v1.size() == 0u);
        CHECK(v2.size() == 1u);
        CHECK(v2[0] == 42);
    }

    SECTION("many elements")
    {
        const auto n = 666u;
        auto v = rvektor<unsigned>{};
        for (auto i = 0u; i < n; ++i) {
            v = v.push_back(i * 42);
            CHECK(v.size() == i + 1);
            for (auto j = 0u; j < v.size(); ++j)
                CHECK(v[j] == j * 42);
        }
    }

    SECTION("many elements, small branching factor")
    {
        const auto n = 666u;
        auto v = rvektor<unsigned, 3>{};
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
    auto v = make_test_rvektor(0, n);

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
        auto v = make_test_rvektor(0, 666u);
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
        auto v = make_test_rvektor<4>(0, 1000u);
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

TEST_CASE("push_front")
{
    const auto n = 666u;
    auto v = rvektor<unsigned, 3>{};

    for (auto i = 0u; i < n; ++i) {
        IMMU_TRACE("\n-- push_front: " << i);
        v = v.push_front(i);
        CHECK(v.size() == i + 1);
        for (auto j = 0; j < v.size(); ++j)
            CHECK(v[v.size() - j - 1] == j);
    }
}

TEST_CASE("concat")
{
    const auto n = 666u;

    auto all_lhs = make_many_test_rvektor<3>(n);
    auto all_rhs = make_many_test_rvektor_front_remainder<3>(n);

    SECTION("anywhere")
    {
        for (auto i = 0u; i < n; ++i) {
            auto c = all_lhs[i] + all_rhs[n - i - 1];
            IMMU_TRACE("\n-- concat: " << i);
            CHECK(c.size() == n - 1);
            for (auto j = 0u; j < c.size(); ++j)
                CHECK(c[j] == j);
        }
    }
}

TEST_CASE("reduce")
{
    SECTION("sum regular")
    {
        const auto n = 666u;
        auto v = make_test_rvektor(0, n);

        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = v.size() * (v.size() - 1) / 2;
        CHECK(sum == expected);
    }

    SECTION("sum relaxed")
    {
        const auto n = 666u;
        auto v = make_test_rvektor_front(0, n);

        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = v.size() * (v.size() - 1) / 2;
        CHECK(sum == expected);
    }

    SECTION("sum relaxed complex")
    {
        const auto n = 20u;

        auto v  = rvektor<unsigned, 3>{};
        for (auto i = 0u; i < n; ++i) {
            IMMU_TRACE("\n-- sum relaxed complex: " << i << " | " << v.size());
            v = v.push_front(i) + v;
        }
        /*
          0  // 0
          1  // 1 0 0
          4  // 2 1 0 0 1 0 0
          11 // 3 2 1 0 0 1 0 0 2 1 0 0 1 0 0
          26 // 4 3 2 1 0 0 1 0 0 2 1 0 0 1 0 0
        */
        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = (1 << n) - n - 1;
        CHECK(sum == expected);
    }
}

TEST_CASE("take")
{
    const auto n = 666u;

    SECTION("anywhere")
    {
        auto v = make_test_rvektor<3>(0, n);

        for (auto i = 0u; i < n; ++i) {
            auto vv = v.take(i);
            CHECK(vv.size() == i);
            for (auto j = 0u; j < i; ++j)
                CHECK(vv[j] == v[j]);
        }
    }

    SECTION("relaxed")
    {
        auto v = make_test_rvektor_front<3>(0, n);

        for (auto i = 0u; i < n; ++i) {
            auto vv = v.take(i);
            CHECK(vv.size() == i);
            for (auto j = 0u; j < i; ++j)
                CHECK(vv[j] == v[j]);
        }
    }
}

TEST_CASE("drop")
{
    const auto n = 666u;

    SECTION("regular")
    {
        auto v = make_test_rvektor<3>(0, n);

        for (auto i = 0u; i < n; ++i) {
            auto vv = v.drop(i);
            CHECK(vv.size() == n - i);
            for (auto j = 0u; j < n - i; ++j)
                CHECK(vv[j] == v[j + i]);
        }
    }

    SECTION("relaxed")
    {
        auto v = make_test_rvektor_front<3>(0, n);

        for (auto i = 0u; i < n; ++i) {
            auto vv = v.drop(i);
            CHECK(vv.size() == n - i);
            for (auto j = 0u; j < n - i; ++j)
                CHECK(vv[j] == v[j + i]);
        }
    }
}
