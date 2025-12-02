#include <immer/box.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/table.hpp>
#include <immer/vector.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

// 16-byte alignment seems to happen by default for box, map, set, is
// only broken by 32, but that incurs over-alignment which requires a custom
// allocator anyway...
constexpr unsigned alignment = 16;

// just in case alignment issues only show up for the non-first element
constexpr int N = 4;

// A structure with the same alignment requirements as Eigen's vectorized types
// (which is what originally exposed this issue)
struct AlignConstrainedType
{
    alignas(alignment) std::array<double, 4> buf = {{0, 0, 0, 0}};

    AlignConstrainedType() = default;

    // For the set case
    AlignConstrainedType(double k)
        : buf{{k, k, k, k}} {};

    // For the set case
    bool operator==(const AlignConstrainedType& o) const
    {
        return o.buf == buf;
    }

    // https://stackoverflow.com/a/42093940
    bool is_aligned() const
    {
        const auto rem = reinterpret_cast<intptr_t>(this) % alignment;
        return !rem;
    }
};
static_assert(alignof(AlignConstrainedType) == alignment, "bad alignment");
static_assert(alignof(AlignConstrainedType) <= alignof(max_align_t),
              "bad size alignment");

// For the set case
namespace std {
template <>
struct hash<AlignConstrainedType>
{
    size_t operator()(const AlignConstrainedType& v) const
    {
        return v.buf[0] + v.buf[1] + v.buf[2] +
               v.buf[3]; // terrible but valid hash
    };
};
} // namespace std

// For the table case
struct TableElem
{
    int id;
    AlignConstrainedType value;
};
static_assert(alignof(TableElem) == alignment, "bad alignment");

TEST_CASE("Sanity check: std::vector")
{
    std::vector<AlignConstrainedType> v;
    for (int i = 0; i < N; ++i) {
        v.emplace_back();
    }

    for (const auto& elem : v) {
        CHECK(elem.is_aligned());
    }
}

TEST_CASE("Simple alignment, vector")
{
    immer::vector<AlignConstrainedType> v;

    for (int i = 0; i < N; ++i) {
        v = std::move(v).push_back({});
    }

    for (const auto& elem : v) {
        CHECK(elem.is_aligned());
    }
}

TEST_CASE("Simple alignment, vector of box")
{
    // Checking many boxes just in case one of them happens to align
    immer::vector<immer::box<AlignConstrainedType>> v;

    for (int i = 0; i < N; ++i) {
        v = std::move(v).push_back({});
    }

    for (const auto& elem : v) {
        CHECK(elem->is_aligned());
    }
}

TEST_CASE("Simple alignment, flex_vector")
{
    immer::flex_vector<AlignConstrainedType> v;

    for (int i = 0; i < N; ++i) {
        v = std::move(v).push_back({});
    }

    for (const auto& elem : v) {
        CHECK(elem.is_aligned());
    }
}

TEST_CASE("Simple alignment, set")
{
    immer::set<AlignConstrainedType> v;

    for (int i = 0; i < N; ++i) {
        v = std::move(v).insert({static_cast<double>(i)});
    }

    for (const auto& elem : v) {
        CHECK(elem.is_aligned());
    }
}

TEST_CASE("Simple alignment, map")
{
    immer::map<int, AlignConstrainedType> v;

    for (int i = 0; i < N; ++i) {
        v = std::move(v).insert({i, AlignConstrainedType{}});
    }

    for (const auto& elem : v) {
        CHECK(elem.second.is_aligned());
    }
}

TEST_CASE("Simple alignment, table")
{
    immer::table<TableElem> v;

    for (int i = 0; i < N; ++i) {
        v = std::move(v).insert({i, AlignConstrainedType{}});
    }

    for (const auto& elem : v) {
        CHECK(elem.value.is_aligned());
    }
}
