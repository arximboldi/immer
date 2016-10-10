
#pragma once

#include <boost/range/irange.hpp>
#include <boost/range/join.hpp>

#if IMMER_SLOW_TESTS
#define CHECK_VECTOR_EQUALS_RANGE(v1_, first_, last_)   \
    [] (auto&& v1, auto&& first, auto&& last) {         \
        auto size = std::distance(first, last);         \
        CHECK(v1.size() == size);                       \
        for (auto j = 0u; j < size; ++j)                \
            CHECK(v1[j] == *first++);                   \
    } (v1_, first_, last_)                              \
    // CHECK_EQUALS
#else
#define CHECK_VECTOR_EQUALS_RANGE(v1_, first_, last_)                   \
    [] (auto&& v1, auto&& first, auto&& last) {                         \
        auto size = std::distance(first, last);                         \
        CHECK(v1.size() == size);                                       \
        if (size > 0) {                                                 \
            CHECK(v1[0] == *(first + (0)));                             \
            CHECK(v1[size - 1] == *(first + (size - 1)));               \
            CHECK(v1[size / 2] == *(first + (size / 2)));               \
            CHECK(v1[size / 3] == *(first + (size / 3)));               \
            CHECK(v1[size / 4] == *(first + (size / 4)));               \
            CHECK(v1[size - 1 - size / 2] == *(first + (size - 1 - size / 2))); \
            CHECK(v1[size - 1 - size / 3] == *(first + (size - 1 - size / 3))); \
            CHECK(v1[size - 1 - size / 4] == *(first + (size - 1 - size / 4))); \
        }                                                               \
        if (size > 1) {                                                 \
            CHECK(v1[1] == *(first + (1)));                             \
            CHECK(v1[size - 2] == *(first + (size - 2)));               \
        }                                                               \
        if (size > 2) {                                                 \
            CHECK(v1[2] == *(first + (2)));                             \
            CHECK(v1[size - 3] == *(first + (size - 3)));               \
        }                                                               \
    } (v1_, first_, last_)                                              \
    // CHECK_EQUALS
#endif // IMMER_SLOW_TESTS

#define CHECK_VECTOR_EQUALS(v1, v2)                             \
    CHECK_VECTOR_EQUALS_RANGE((v1), (v2).begin(), (v2).end());

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
