
#include <immer/flex_vector.hpp>

#include <catch.hpp>
#include <boost/range/adaptors.hpp>

#include <algorithm>
#include <numeric>
#include <vector>

using namespace immer;

namespace {

template <unsigned B=5>
auto make_test_flex_vector(std::size_t min, std::size_t max)
{
    auto v = flex_vector<unsigned, B>{};
    for (auto i = min; i < max; ++i)
        v = v.push_back(i);
    return v;
}

template <unsigned B=5>
auto make_test_flex_vector_front(std::size_t min, std::size_t max)
{
    auto v = flex_vector<unsigned, B>{};
    for (auto i = max; i > min;)
        v = v.push_front(--i);
    return v;
}

template <unsigned B=5>
auto make_many_test_flex_vector(std::size_t n)
{
    using vektor_t = flex_vector<unsigned, B>;
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
auto make_many_test_flex_vector_front(std::size_t n)
{
    using vektor_t = flex_vector<unsigned, B>;
    auto many = std::vector<vektor_t>{};
    many.reserve(n);
    std::generate_n(std::back_inserter(many), n,
                    [i = 0u] () mutable
                    {
                        return make_test_flex_vector_front<B>(0, i++);
                    });
    return many;
}

template <unsigned B=5>
auto make_many_test_flex_vector_front_remainder(std::size_t n)
{
    using vektor_t = flex_vector<unsigned, B>;
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
    auto v = flex_vector<int>{};
    CHECK(v.size() == 0u);
}

TEST_CASE("push_back")
{
    SECTION("one element")
    {
        const auto v1 = flex_vector<int>{};
        auto v2 = v1.push_back(42);
        CHECK(v1.size() == 0u);
        CHECK(v2.size() == 1u);
        CHECK(v2[0] == 42);
    }

    SECTION("many elements")
    {
        const auto n = 666u;
        auto v = flex_vector<unsigned>{};
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
        auto v = flex_vector<unsigned, 3>{};
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
    auto v = make_test_flex_vector(0, n);

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
        auto v = make_test_flex_vector(0, 666u);
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
        auto v = make_test_flex_vector<4>(0, 1000u);
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
    auto v = flex_vector<unsigned, 3>{};

    for (auto i = 0u; i < n; ++i) {
        IMMER_TRACE("\n-- push_front: " << i);
        v = v.push_front(i);
        CHECK(v.size() == i + 1);
        for (auto j = 0u; j < v.size(); ++j)
            CHECK(v[v.size() - j - 1] == j);
    }
}

TEST_CASE("concat")
{
    const auto n = 666u;

    auto all_lhs = make_many_test_flex_vector<3>(n);
    auto all_rhs = make_many_test_flex_vector_front_remainder<3>(n);

    SECTION("anywhere")
    {
        for (auto i = 0u; i < n; ++i) {
            auto c = all_lhs[i] + all_rhs[n - i - 1];
            IMMER_TRACE("\n-- concat: " << i);
            CHECK(c.size() == n - 1);
            for (auto j = 0u; j < c.size(); ++j)
                CHECK(c[j] == j);
        }
    }
}

template <unsigned B=5>
auto make_flex_vector_concat(std::size_t min, std::size_t max)
{
    using vektor_t = flex_vector<unsigned, B>;
    if (max == min)
        return vektor_t{};
    else if (max == min + 1)
        return vektor_t{}.push_back(min);
    auto mid = min + (max - min) / 2;
    return make_flex_vector_concat<B>(min, mid)
        +  make_flex_vector_concat<B>(mid, max);
}

TEST_CASE("concat recursive")
{
    const auto n = 666u;
    auto v = make_flex_vector_concat<3>(0, n);

    CHECK(v.size() == n);
    for (auto i = 0u; i < n; ++i)
        CHECK(v[i] == i);
}

TEST_CASE("reduce")
{
    SECTION("sum regular")
    {
        const auto n = 666u;
        auto v = make_test_flex_vector(0, n);

        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = v.size() * (v.size() - 1) / 2;
        CHECK(sum == expected);
    }

    SECTION("sum relaxed")
    {
        const auto n = 666u;
        auto v = make_test_flex_vector_front(0, n);

        auto sum = v.reduce(std::plus<unsigned>{}, 0u);
        auto expected = v.size() * (v.size() - 1) / 2;
        CHECK(sum == expected);
    }

    SECTION("sum relaxed complex")
    {
        const auto n = 20u;

        auto v  = flex_vector<unsigned, 3>{};
        for (auto i = 0u; i < n; ++i) {
            IMMER_TRACE("\n-- sum relaxed complex: " << i << " | " << v.size());
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
        auto v = make_test_flex_vector<3>(0, n);

        for (auto i = 0u; i < n; ++i) {
            auto vv = v.take(i);
            CHECK(vv.size() == i);
            for (auto j = 0u; j < i; ++j)
                CHECK(vv[j] == v[j]);
        }
    }

    SECTION("relaxed")
    {
        auto v = make_test_flex_vector_front<3>(0, n);

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
        auto v = make_test_flex_vector<3>(0, n);

        for (auto i = 0u; i < n; ++i) {
            auto vv = v.drop(i);
            CHECK(vv.size() == n - i);
            for (auto j = 0u; j < n - i; ++j)
                CHECK(vv[j] == v[j + i]);
        }
    }

    SECTION("relaxed")
    {
        auto v = make_test_flex_vector_front<3>(0, n);

        for (auto i = 0u; i < n; ++i) {
            auto vv = v.drop(i);
            CHECK(vv.size() == n - i);
            for (auto j = 0u; j < n - i; ++j)
                CHECK(vv[j] == v[j + i]);
        }
    }
}

TEST_CASE("reconcat")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front<3>(0, n);
    auto all_lhs = make_many_test_flex_vector_front<3>(n + 1);
    auto all_rhs = make_many_test_flex_vector_front_remainder<3>(n + 1);

    for (auto i = 0u; i < n; ++i) {
        auto vv = all_lhs[i] + all_rhs[n - i];
        CHECK(vv.size() == n);
        for (auto j = 0u; j < n; ++j)
            CHECK(vv[j] == v[j]);
    }
}

TEST_CASE("reconcat drop")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front<3>(0, n);
    auto all_lhs = make_many_test_flex_vector_front<3>(n + 1);

    for (auto i = 0u; i < n; ++i) {
        auto vv = all_lhs[i] + v.drop(i);
        CHECK(vv.size() == n);
        for (auto j = 0u; j < n; ++j)
            CHECK(vv[j] == v[j]);
    }
}

TEST_CASE("reconcat take")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front<3>(0, n);
    auto all_rhs = make_many_test_flex_vector_front_remainder<3>(n + 1);

    for (auto i = 0u; i < n; ++i) {
        auto vv = v.take(i) + all_rhs[n - i];
        CHECK(vv.size() == n);
        for (auto j = 0u; j < n; ++j)
            CHECK(vv[j] == v[j]);
    }
}

TEST_CASE("reconcat take drop")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front<3>(0, n);

    for (auto i = 0u; i < n; ++i) {
        auto vv = v.take(i) + v.drop(i);
        CHECK(vv.size() == n);
        for (auto j = 0u; j < n; ++j)
            CHECK(vv[j] == v[j]);
    }
}

TEST_CASE("reconcat take drop feedback")
{
    const auto n = 666u;
    auto v = make_test_flex_vector_front<3>(0, n);
    auto vv = v;
    for (auto i = 0u; i < n; ++i) {
        vv = vv.take(i) + vv.drop(i);
        CHECK(vv.size() == n);
        for (auto j = 0u; j < n; ++j)
            CHECK(vv[j] == v[j]);

TEST_CASE("iterator")
{
    const auto n = 666u;
    auto v = make_test_flex_vector(0, n);

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
