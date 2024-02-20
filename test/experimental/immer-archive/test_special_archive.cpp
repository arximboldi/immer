#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include "utils.hpp"

#include <boost/hana.hpp>
#include <immer/experimental/immer-archive/box/archive.hpp>
#include <immer/experimental/immer-archive/champ/traits.hpp>
#include <immer/experimental/immer-archive/json/archivable.hpp>
#include <immer/experimental/immer-archive/json/json_with_archive.hpp>
#include <immer/experimental/immer-archive/rbts/traits.hpp>

#include <immer/box.hpp>

// to save std::pair
#include <cereal/types/utility.hpp>

#include <boost/hana/ext/std/tuple.hpp>

namespace {

namespace hana = boost::hana;

/**
 * Some user data type that contains some vector_one_archivable, which should be
 * serialized in a special way.
 */

using test::flex_vector_one;
using test::test_value;
using test::vector_one;

template <class T>
using arch = immer_archive::archivable<T>;

template <class T>
std::string string_via_tie(const T& value)
{
    std::string result;
    hana::for_each(value.tie(), [&](const auto& item) {
        using Item = std::decay_t<decltype(item)>;
        result += (result.empty() ? "" : ", ") +
                  Catch::StringMaker<Item>::convert(item);
    });
    return result;
}

struct meta_meta
{
    arch<vector_one<int>> ints;
    arch<immer::table<test_value>> table;

    auto tie() const { return std::tie(ints, table); }

    friend bool operator==(const meta_meta& left, const meta_meta& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const meta_meta& value)
    {
        return s << string_via_tie(value);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(table));
    }
};

struct meta
{
    arch<vector_one<int>> ints;
    arch<vector_one<meta_meta>> metas;

    auto tie() const { return std::tie(ints, metas); }

    friend bool operator==(const meta& left, const meta& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const meta& value)
    {
        return s << string_via_tie(value);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(metas));
    }
};

struct test_data
{
    arch<vector_one<int>> ints;
    arch<vector_one<std::string>> strings;

    arch<flex_vector_one<int>> flex_ints;
    arch<immer::map<int, std::string>> map;

    arch<vector_one<meta>> metas;

    // Map value is indirectly archivable
    arch<immer::map<int, meta>> metas_map;

    // Map value is directly archivable
    arch<immer::map<int, arch<vector_one<int>>>> vectors_map;

    // Also test having meta directly, not inside an archivable type
    meta single_meta;

    arch<immer::box<std::string>> box;

    auto tie() const
    {
        return std::tie(ints,
                        strings,
                        flex_ints,
                        map,
                        metas,
                        metas_map,
                        vectors_map,
                        single_meta,
                        box);
    }

    friend bool operator==(const test_data& left, const test_data& right)
    {
        return left.tie() == right.tie();
    }

    /**
     * Serialization function is defined as normal.
     */
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints),
           CEREAL_NVP(strings),
           CEREAL_NVP(flex_ints),
           CEREAL_NVP(map),
           CEREAL_NVP(metas),
           CEREAL_NVP(metas_map),
           CEREAL_NVP(vectors_map),
           CEREAL_NVP(single_meta),
           CEREAL_NVP(box));
    }
};

/**
 * A special function that enumerates which types of immer-archives are
 * required. Explicitly name each type, for simplicity.
 */
inline auto get_archives_types(const test_data&)
{
    auto names = hana::make_map(
        hana::make_pair(hana::type_c<vector_one<int>>,
                        BOOST_HANA_STRING("ints")),
        hana::make_pair(hana::type_c<vector_one<std::string>>,
                        BOOST_HANA_STRING("strings")),
        hana::make_pair(hana::type_c<flex_vector_one<int>>,
                        BOOST_HANA_STRING("flex_ints")),
        hana::make_pair(hana::type_c<immer::map<int, std::string>>,
                        BOOST_HANA_STRING("int_string_map")),
        hana::make_pair(hana::type_c<vector_one<meta>>,
                        BOOST_HANA_STRING("metas")),
        hana::make_pair(hana::type_c<vector_one<meta_meta>>,
                        BOOST_HANA_STRING("meta_metas")),
        hana::make_pair(hana::type_c<immer::table<test_value>>,
                        BOOST_HANA_STRING("table_test_value")),
        hana::make_pair(hana::type_c<immer::map<int, meta>>,
                        BOOST_HANA_STRING("int_meta_map")),
        hana::make_pair(hana::type_c<immer::map<int, arch<vector_one<int>>>>,
                        BOOST_HANA_STRING("int_vector_map")),
        hana::make_pair(hana::type_c<immer::box<std::string>>,
                        BOOST_HANA_STRING("string_box"))

    );
    return names;
}

inline auto get_archives_types(const std::pair<test_data, test_data>&)
{
    return get_archives_types(test_data{});
}

} // namespace

template <>
struct fmt::formatter<meta_meta> : ostream_formatter
{};

template <>
struct fmt::formatter<meta> : ostream_formatter
{};

namespace Catch {
template <>
struct StringMaker<test_data>
{
    static std::string convert(const test_data& value)
    {
        return string_via_tie(value);
    }
};

template <>
struct StringMaker<meta>
{
    static std::string convert(const meta& value)
    {
        return string_via_tie(value);
    }
};

template <>
struct StringMaker<meta_meta>
{
    static std::string convert(const meta_meta& value)
    {
        return string_via_tie(value);
    }
};
} // namespace Catch

TEST_CASE("Special archive minimal test")
{
    const auto ints1 = vector_one<int>{
        1,
        2,
        3,
        4,
        5,
    };
    const auto meta_value = meta{
        .ints = ints1,
        .metas =
            {
                meta_meta{
                    .ints = ints1,
                },
            },
    };
    const auto test1 = test_data{
        .metas =
            {
                meta_value,
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
    };

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(test1);

    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer_archive::from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer_archive::to_json_with_archive(full_load).first == "");
    }
}

TEST_CASE("Save with a special archive")
{
    spdlog::set_level(spdlog::level::debug);

    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map =
            {
                {1, "_one_"},
                {2, "two__"},
            },
        .metas = {vector_one<meta>{
            meta{
                .ints =
                    {
                        1,
                        2,
                        3,
                        4,
                        5,
                    },
            },
            meta{
                .ints = ints1,
            },
        }},
        .single_meta =
            meta{
                .ints = {66, 50, 55},
            },
    };

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(test1);
    SECTION("Try to save and load the archive")
    {
        const auto archives_json = [&archives = archives] {
            auto os = std::ostringstream{};
            {
                auto ar = immer_archive::json_immer_output_archive<
                    std::decay_t<decltype(archives)>>{os};
                ar(123);
                ar(CEREAL_NVP(archives));
            }
            return os.str();
        }();
        // REQUIRE(archives_json == "");
        const auto archives_loaded = [&archives_json] {
            using Archives =
                decltype(immer_archive::detail::generate_archives_load(
                    get_archives_types(test_data{})));
            auto archives = Archives{};

            {
                auto is = std::istringstream{archives_json};
                auto ar = cereal::JSONInputArchive{is};
                ar(CEREAL_NVP(archives));
            }

            {
                auto is = std::istringstream{archives_json};
                auto ar = immer_archive::json_immer_input_archive<Archives>{
                    archives, is};
                ar(CEREAL_NVP(archives));
            }

            return archives;
        }();
        REQUIRE(archives_loaded.storage[hana::type_c<vector_one<int>>]
                    .archive.leaves.size() == 7);
    }

    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer_archive::from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer_archive::to_json_with_archive(full_load).first == "");
    }
}

TEST_CASE("Save with a special archive, special type is enclosed")
{
    const auto map = immer::map<int, std::string>{
        {1, "_one_"},
        {2, "two__"},
    };
    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto ints5 = vector_one<int>{
        1,
        2,
        3,
        4,
        5,
    };
    const auto metas = vector_one<meta>{
        meta{
            .ints = ints5,
        },
        meta{
            .ints = ints1,
        },
    };
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map       = map,
        .metas     = metas,
    };
    const auto test2 = test_data{
        .ints      = ints1,
        .strings   = {"three"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map       = map.set(3, "__three"),
        .metas =
            {
                meta{
                    .ints = ints5,
                },
            },
    };

    // At the beginning, the vector is shared, it's the same data.
    REQUIRE(test1.ints.container.identity() == test2.ints.container.identity());
    REQUIRE(test1.flex_ints.container.identity() ==
            test2.flex_ints.container.identity());

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(std::make_pair(test1, test2));

    // REQUIRE(json_str == "");

    {
        auto [loaded1, loaded2] = immer_archive::from_json_with_archive<
            std::pair<test_data, test_data>>(json_str);
        REQUIRE(loaded1 == test1);
        REQUIRE(loaded2 == test2);

        REQUIRE(loaded1.metas.container.size() == 2);
        REQUIRE(loaded1.metas.container[0].ints == ints5);
        REQUIRE(loaded1.metas.container[1].ints == ints1);

        // After loading, two vectors are still reused.
        REQUIRE(loaded1.ints.container.identity() ==
                loaded2.ints.container.identity());
        REQUIRE(loaded1.flex_ints.container.identity() ==
                loaded2.flex_ints.container.identity());

        REQUIRE(loaded1.metas.container[0].ints.container.identity() ==
                loaded2.metas.container[0].ints.container.identity());
    }
}

TEST_CASE("Special archive must load and save types that have no archive")
{
    const auto val1  = test_value{123, "value1"};
    const auto val2  = test_value{234, "value2"};
    const auto value = std::make_pair(val1, val2);

    const auto json_archive_str =
        immer_archive::to_json_with_archive(value).first;
    REQUIRE(json_archive_str == test::to_json(value));

    {
        auto loaded = immer_archive::from_json_with_archive<
            std::decay_t<decltype(value)>>(json_archive_str);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special archive loads empty test_data")
{
    const auto value = test_data{};

    // const auto json_archive_str =
    //     immer_archive::to_json_with_archive(value).first;
    // REQUIRE(json_archive_str == "");

    const auto json_archive_str = R"({
  "value0": {
    "ints": 0,
    "strings": 0,
    "flex_ints": 0,
    "map": 0,
    "metas": 0,
    "metas_map": 0,
    "vectors_map": 0,
    "single_meta": {"ints": 0, "metas": 0},
    "box": 0
  },
  "archives": {
    "ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "strings": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "flex_ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "int_string_map": [
          {"values": [], "children": [], "nodemap": 0, "datamap": 0, "collisions": false}
    ],
    "metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "meta_metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "table_test_value": [],
    "int_meta_map": [
        {"values": [], "children": [], "nodemap": 0, "datamap": 0, "collisions": false}
    ],
    "int_vector_map": [
        {"values": [], "children": [], "nodemap": 0, "datamap": 0, "collisions": false}
    ],
    "string_box": [""]
  }
})";

    {
        auto loaded = immer_archive::from_json_with_archive<
            std::decay_t<decltype(value)>>(json_archive_str);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special archive throws cereal::Exception")
{
    const auto value = test_data{};

    // const auto json_archive_str =
    //     immer_archive::to_json_with_archive(value).first;
    // REQUIRE(json_archive_str == "");

    const auto json_archive_str = R"({
  "value0": {
    "ints": 99,
    "strings": 0,
    "flex_ints": 0,
    "map": 0,
    "metas": 0,
    "single_meta": {"ints": 0, "metas": 0}
  },
  "archives": {
    "ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "strings": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "flex_ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "int_string_map": [
        {"values": [], "children": [], "nodemap": 0, "datamap": 0, "collisions": false}
    ],
    "metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "meta_metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"root": 0, "tail": 1}]
    },
    "table_test_value": [],
    "int_meta_map": [],
    "int_vector_map": [],
    "string_box": []
  }
})";

    REQUIRE_THROWS_MATCHES(
        immer_archive::from_json_with_archive<test_data>(json_archive_str),
        ::cereal::Exception,
        Catch::Matchers::Message("Failed to load a container ID 99 from the "
                                 "archive: Container ID 99 is not found"));
}

namespace {
struct recursive_type
{
    int data;
    arch<vector_one<arch<immer::box<recursive_type>>>> children;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(data), CEREAL_NVP(children));
    }

    auto tie() const { return std::tie(data, children); }

    friend bool operator==(const recursive_type& left,
                           const recursive_type& right)
    {
        return left.tie() == right.tie();
    }
};

auto get_archives_types(const recursive_type&)
{
    auto names = hana::make_map(
        hana::make_pair(hana::type_c<immer::box<recursive_type>>,
                        BOOST_HANA_STRING("boxes")),
        hana::make_pair(
            hana::type_c<vector_one<arch<immer::box<recursive_type>>>>,
            BOOST_HANA_STRING("vectors"))

    );
    return names;
}
} // namespace

TEST_CASE("Test recursive type")
{
    const auto v1 = recursive_type{
        .data = 123,
    };
    const auto v2 = recursive_type{
        .data = 234,
    };
    const auto v3 = recursive_type{
        .data     = 345,
        .children = {immer::box<recursive_type>{v1},
                     immer::box<recursive_type>{v2}},
    };

    const auto [json_str, archives] = immer_archive::to_json_with_archive(v3);
    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer_archive::from_json_with_archive<recursive_type>(json_str);
        REQUIRE(full_load == v3);
        REQUIRE(immer_archive::to_json_with_archive(full_load).first ==
                json_str);
    }
}

TEST_CASE("Test recursive type, saving the box triggers saving the box of the "
          "same type")
{
    const auto v1 = recursive_type{
        .data = 123,
    };
    const auto v2 = recursive_type{
        .data = 234,
    };
    const auto v3 = recursive_type{
        .data     = 345,
        .children = {immer::box<recursive_type>{v1},
                     immer::box<recursive_type>{v2}},
    };
    // NOTE: v3 is boxed and inside of it, it contains more boxed values.
    const auto v4 = recursive_type{
        .data     = 456,
        .children = {immer::box<recursive_type>{v1},
                     immer::box<recursive_type>{v3}},
    };
    const auto v5 = recursive_type{
        .data = 567,
        .children =
            {
                immer::box<recursive_type>{v4},
            },
    };

    const auto [json_str, archives] = immer_archive::to_json_with_archive(v5);
    // REQUIRE(json_str == "");

    {
        const auto full_load =
            immer_archive::from_json_with_archive<recursive_type>(json_str);
        REQUIRE(full_load == v5);
        REQUIRE(immer_archive::to_json_with_archive(full_load).first ==
                json_str);
    }
}

namespace {
struct rec_map
{
    int data;
    arch<immer::map<int, arch<immer::box<rec_map>>>> map;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(data), CEREAL_NVP(map));
    }

    auto tie() const { return std::tie(data, map); }

    friend bool operator==(const rec_map& left, const rec_map& right)
    {
        return left.tie() == right.tie();
    }
};

auto get_archives_types(const rec_map&)
{
    auto names = hana::make_map(
        hana::make_pair(hana::type_c<immer::box<rec_map>>,
                        BOOST_HANA_STRING("boxes")),
        hana::make_pair(
            hana::type_c<immer::map<int, arch<immer::box<rec_map>>>>,
            BOOST_HANA_STRING("maps"))

    );
    return names;
}
} // namespace

TEST_CASE("Test saving a map that contains the same map")
{
    const auto v1    = immer::box<rec_map>{rec_map{
           .data = 123,
    }};
    const auto v2    = immer::box<rec_map>{rec_map{
           .data = 234,
           .map =
               {
                {1, v1},
            },
    }};
    const auto value = rec_map{
        .data = 345,
        .map =
            {
                {2, v2},
            },
    };

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(value);
    // REQUIRE(json_str == "");

    {
        const auto full_load =
            immer_archive::from_json_with_archive<rec_map>(json_str);
        REQUIRE(full_load == value);
        REQUIRE(immer_archive::to_json_with_archive(full_load).first ==
                json_str);
    }
}

TEST_CASE("Test non-unique names in the map")
{
    {
        const auto names = hana::make_map(
            hana::make_pair(hana::type_c<immer::box<int>>,
                            BOOST_HANA_STRING("box")),
            hana::make_pair(hana::type_c<immer::box<std::string>>,
                            BOOST_HANA_STRING("box"))

        );

        using IsUnique =
            decltype(immer_archive::detail::are_type_names_unique(names));
        static_assert(boost::hana::value<IsUnique>() == false,
                      "Detect non-unique names");
    }
    {
        const auto names = hana::make_map(
            hana::make_pair(hana::type_c<immer::box<int>>,
                            BOOST_HANA_STRING("box_1")),
            hana::make_pair(hana::type_c<immer::box<std::string>>,
                            BOOST_HANA_STRING("box_2"))

        );

        using IsUnique =
            decltype(immer_archive::detail::are_type_names_unique(names));
        static_assert(boost::hana::value<IsUnique>(), "Names are unique");
    }
}
