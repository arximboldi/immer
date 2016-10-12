
#pragma once

#include <boost/range/irange.hpp>
#include <boost/range/join.hpp>

namespace {

struct identity_t
{
    template <typename T>
    decltype(auto) operator() (T&& x)
    {
        return std::forward<decltype(x)>(x);
    }
};

} // anonymous namespace

#if IMMER_SLOW_TESTS
#define CHECK_VECTOR_EQUALS_RANGE_X(v1_, first_, last_, xf_)      \
    [] (auto&& v1, auto&& first, auto&& last, auto&& xf) {        \
        auto size = std::distance(first, last);                   \
        CHECK(v1.size() == size);                                 \
        for (auto j = 0u; j < size; ++j)                          \
            CHECK(xf(v1[j]) == xf(*first++));                     \
    } (v1_, first_, last_, xf_)                                   \
    // CHECK_EQUALS
#else
#define CHECK_VECTOR_EQUALS_RANGE_X(v1_, first_, last_, ...)            \
    [] (auto&& v1, auto&& first, auto&& last, auto&& xf) {              \
        auto size = std::distance(first, last);                         \
        CHECK(v1.size() == size);                                       \
        if (size > 0) {                                                 \
            CHECK(xf(v1[0]) == xf(*(first + (0))));                     \
            CHECK(xf(v1[size - 1]) == xf(*(first + (size - 1))));       \
            CHECK(xf(v1[size / 2]) == xf(*(first + (size / 2))));       \
            CHECK(xf(v1[size / 3]) == xf(*(first + (size / 3))));       \
            CHECK(xf(v1[size / 4]) == xf(*(first + (size / 4))));       \
            CHECK(xf(v1[size - 1 - size / 2]) == xf(*(first + (size - 1 - size / 2)))); \
            CHECK(xf(v1[size - 1 - size / 3]) == xf(*(first + (size - 1 - size / 3)))); \
            CHECK(xf(v1[size - 1 - size / 4]) == xf(*(first + (size - 1 - size / 4)))); \
        }                                                               \
        if (size > 1) {                                                 \
            CHECK(xf(v1[1]) == xf(*(first + (1))));                     \
            CHECK(xf(v1[size - 2]) == xf(*(first + (size - 2))));       \
        }                                                               \
        if (size > 2) {                                                 \
            CHECK(xf(v1[2]) == xf(*(first + (2))));                     \
            CHECK(xf(v1[size - 3]) == xf(*(first + (size - 3))));       \
        }                                                               \
    } (v1_, first_, last_, __VA_ARGS__)                                 \
    // CHECK_EQUALS
#endif // IMMER_SLOW_TESTS

#define CHECK_VECTOR_EQUALS_X(v1, v2, ...)                              \
    CHECK_VECTOR_EQUALS_RANGE_X((v1), (v2).begin(), (v2).end(), __VA_ARGS__);

#define CHECK_VECTOR_EQUALS_RANGE(v1, b, e)                     \
    CHECK_VECTOR_EQUALS_RANGE_X((v1), (b), (e), identity_t{});

#define CHECK_VECTOR_EQUALS(v1, v2)                             \
    CHECK_VECTOR_EQUALS_X((v1), (v2), identity_t{});

namespace {

template <typename Integer>
auto test_irange(Integer from, Integer to)
{
#if IMMER_SLOW_TESTS
    return boost::irange(from, to);
#else
    assert(to - from > Integer{2});
    return boost::join(
        boost::irange(from, from + Integer{2}),
        boost::join(
            boost::irange(from + Integer{2},
                          to - Integer{2},
                          (to - from) / Integer{5}),
            boost::irange(to - Integer{2}, to)));
#endif
}

} // anonymous namespace
