//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <immer/detail/hamts/bits.hpp>

using namespace immer::detail::hamts;

TEST_CASE("popcount_fallback matches expected bit counts")
{
    SECTION("32-bit")
    {
        CHECK(popcount_fallback(std::uint32_t{0}) == 0u);
        CHECK(popcount_fallback(std::uint32_t{0xFFFFFFFFu}) == 32u);
        CHECK(popcount_fallback(std::uint32_t{1}) == 1u);
        CHECK(popcount_fallback(std::uint32_t{0xAAAAAAAAu}) == 16u);
        CHECK(popcount_fallback(std::uint32_t{0x80000000u}) == 1u);
    }

    SECTION("64-bit")
    {
        CHECK(popcount_fallback(std::uint64_t{0}) == 0u);
        CHECK(popcount_fallback(std::uint64_t{0xFFFFFFFFFFFFFFFFull}) == 64u);
        CHECK(popcount_fallback(std::uint64_t{1}) == 1u);
        CHECK(popcount_fallback(std::uint64_t{0xAAAAAAAAAAAAAAAAull}) == 32u);
        CHECK(popcount_fallback(std::uint64_t{0x8000000000000000ull}) == 1u);
    }
}

TEST_CASE("popcount agrees with popcount_fallback")
{
    // popcount() may dispatch to a platform intrinsic (or, on MSVC, a
    // runtime-checked intrinsic -- see has_popcnt_support() in bits.hpp);
    // it must always agree with the portable fallback it is meant to be
    // equivalent to.
    SECTION("8-bit")
    {
        for (unsigned i = 0; i <= 0xFF; ++i) {
            auto x = static_cast<std::uint8_t>(i);
            CHECK(popcount(x) == popcount_fallback(static_cast<std::uint32_t>(x)));
        }
    }

    SECTION("16-bit samples")
    {
        for (auto x : {std::uint16_t{0},
                        std::uint16_t{1},
                        std::uint16_t{0xFFFF},
                        std::uint16_t{0xAAAA},
                        std::uint16_t{0x8001}}) {
            CHECK(popcount(x) ==
                  popcount_fallback(static_cast<std::uint32_t>(x)));
        }
    }

    SECTION("32-bit samples")
    {
        for (auto x : {std::uint32_t{0},
                        std::uint32_t{1},
                        std::uint32_t{0xFFFFFFFFu},
                        std::uint32_t{0xAAAAAAAAu},
                        std::uint32_t{0x80000001u}}) {
            CHECK(popcount(x) == popcount_fallback(x));
        }
    }

    SECTION("64-bit samples")
    {
        for (auto x : {std::uint64_t{0},
                        std::uint64_t{1},
                        std::uint64_t{0xFFFFFFFFFFFFFFFFull},
                        std::uint64_t{0xAAAAAAAAAAAAAAAAull},
                        std::uint64_t{0x8000000000000001ull}}) {
            CHECK(popcount(x) == popcount_fallback(x));
        }
    }
}
