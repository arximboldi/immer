#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include "utils.hpp"

#include <immer/extra/archive/json/json_with_archive_auto.hpp>

namespace {

namespace hana = boost::hana;
using test::test_value;
using test::vector_one;

/**
 * Unexpected thing: we can't use `hana::equal` to implement the operator== for
 * a struct. It ends up calling itself in an endless recursion.
 */
constexpr auto members = [](const auto& value) {
    return hana::keys(value) | [&value](auto key) {
        return hana::make_tuple(hana::at_key(value, key));
    };
};

constexpr auto serialize_members = [](auto& ar, auto& value) {
    hana::for_each(hana::keys(value), [&](auto key) {
        ar(cereal::make_nvp(key.c_str(), hana::at_key(value, key)));
    });
};

/**
 * A data type with immer members, note the absence of `archivable`.
 */
struct meta_meta
{
    BOOST_HANA_DEFINE_STRUCT(meta_meta,
                             (vector_one<int>, ints),
                             (immer::table<test_value>, table));

    friend bool operator==(const meta_meta& left, const meta_meta& right)
    {
        return members(left) == members(right);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

struct meta
{
    BOOST_HANA_DEFINE_STRUCT(meta,
                             (vector_one<int>, ints),
                             (vector_one<meta_meta>, metas)

    );

    friend bool operator==(const meta& left, const meta& right)
    {
        return members(left) == members(right);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

struct test_data_with_immer
{
    BOOST_HANA_DEFINE_STRUCT(
        test_data_with_immer,
        // Contains immer types
        (vector_one<int>, ints),
        (vector_one<std::string>, strings),
        (immer::map<int, std::string>, map),
        (vector_one<meta>, metas),
        // Map value is indirectly archivable
        (immer::map<int, meta>, metas_map),
        // Map value is directly archivable
        (immer::map<int, vector_one<int>>, vectors_map),
        // Also test having meta directly, not inside an archivable type
        (meta, single_meta),
        (immer::box<std::string>, box),

        // And non-immer types
        (std::vector<int>, std_vector_ints)

    );

    friend bool operator==(const test_data_with_immer& left,
                           const test_data_with_immer& right)
    {
        return members(left) == members(right);
    }

    /**
     * Serialization function is defined as normal.
     */
    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};

} // namespace

TEST_CASE("Auto-archiving")
{
    constexpr auto names = [] {
        return immer::archive::get_archives_for_types(
            hana::tuple_t<test_data_with_immer, meta, meta_meta>,
            hana::make_map(hana::make_pair(hana::type_c<vector_one<meta_meta>>,
                                           BOOST_HANA_STRING("meta_meta"))));
    };

    using ArchiveTypes           = decltype(names());
    constexpr auto archive_types = ArchiveTypes{};

    // Verify auto-generated map of names and types
    static_assert(archive_types[hana::type_c<vector_one<int>>] ==
                  BOOST_HANA_STRING("ints"));
    static_assert(archive_types[hana::type_c<immer::map<int, std::string>>] ==
                  BOOST_HANA_STRING("map"));

    const auto ints1 = vector_one<int>{
        1,
        2,
    };
    const auto ints2      = ints1.push_back(3).push_back(4).push_back(5);
    const auto meta_value = meta{
        .ints = ints1,
        .metas =
            {
                meta_meta{
                    .ints  = ints1,
                    .table = {test_value{}},
                },
            },
    };
    const auto value = test_data_with_immer{
        .ints    = ints2,
        .strings = {"one", "two"},
        .map =
            {
                {1, "_one_"},
                {2, "two__"},
            },
        .metas =
            {
                meta_value,
                meta{
                    .ints = ints2,
                },
            },
        .metas_map =
            {
                {234, meta_value},
            },
        .vectors_map =
            {
                {234, {2, 3, 4}},
                {567, {5, 6, 7}},
                {789, ints1},
            },
        .single_meta     = meta_value,
        .box             = std::string{"hello from the box"},
        .std_vector_ints = {4, 5, 6, 7},
    };

    const auto [json_str, archives] =
        immer::archive::to_json_with_auto_archive(value, archive_types);
    // REQUIRE(json_str == "");

    {
        const auto loaded =
            immer::archive::from_json_with_auto_archive<test_data_with_immer>(
                json_str, archive_types);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Auto-archive must load and save types that have no archive")
{
    const auto val1  = test_value{123, "value1"};
    const auto val2  = test_value{234, "value2"};
    const auto value = std::make_pair(val1, val2);

    const auto json_archive_str =
        immer::archive::to_json_with_auto_archive(value, hana::make_map())
            .first;
    REQUIRE(json_archive_str == test::to_json(value));

    {
        auto loaded = immer::archive::from_json_with_auto_archive<
            std::decay_t<decltype(value)>>(json_archive_str, hana::make_map());
        INFO(loaded.first);
        INFO(loaded.second);
        INFO(value.first);
        INFO(value.second);
        REQUIRE(loaded == value);
    }
}

namespace {
// Just a small type for testing
struct test_data_with_one_immer_member
{
    BOOST_HANA_DEFINE_STRUCT(test_data_with_one_immer_member,
                             (vector_one<int>, ints)

    );

    friend bool operator==(const test_data_with_one_immer_member& left,
                           const test_data_with_one_immer_member& right)
    {
        return members(left) == members(right);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        serialize_members(ar, *this);
    }
};
} // namespace

TEST_CASE("Test save and load small type")
{
    const auto ints1 = vector_one<int>{
        1,
        2,
    };
    const auto value = test_data_with_one_immer_member{
        .ints = ints1,
    };
    const auto archive_types = immer::archive::get_auto_archives_types(value);
    const auto [json_str, archives] =
        immer::archive::to_json_with_auto_archive(value, archive_types);
    // REQUIRE(json_str == "");

    {
        const auto loaded = immer::archive::from_json_with_auto_archive<
            test_data_with_one_immer_member>(json_str, archive_types);
        INFO(test::to_json(loaded));
        INFO(test::to_json(value));
        REQUIRE(loaded == value);
    }
}
