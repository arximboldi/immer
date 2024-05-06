#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "utils.hpp"

#include <boost/hana.hpp>
#include <immer/extra/archive/box/archive.hpp>
#include <immer/extra/archive/champ/traits.hpp>
#include <immer/extra/archive/json/archivable.hpp>
#include <immer/extra/archive/json/json_with_archive.hpp>
#include <immer/extra/archive/rbts/traits.hpp>
#include <immer/extra/archive/xxhash/xxhash.hpp>

// to save std::pair
#include <cereal/types/utility.hpp>

#include <boost/hana/ext/std/tuple.hpp>

#include <nlohmann/json.hpp>

namespace {

namespace hana = boost::hana;

/**
 * Some user data type that contains some vector_one_archivable, which should be
 * serialized in a special way.
 */

using test::flex_vector_one;
using test::test_value;
using test::vector_one;
using json_t = nlohmann::json;

template <class T>
using arch = immer::archive::archivable<T>;

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
 * A special function that enumerates which types of archives are
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
        immer::archive::to_json_with_archive(test1);

    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer::archive::from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer::archive::to_json_with_archive(full_load).first == "");
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
        immer::archive::to_json_with_archive(test1);
    SECTION("Try to save and load the archive")
    {
        const auto archives_json = [&archives = archives] {
            auto os = std::ostringstream{};
            {
                auto ar = immer::archive::json_immer_output_archive<
                    std::decay_t<decltype(archives)>>{os};
                ar(123);
                ar(CEREAL_NVP(archives));
            }
            return os.str();
        }();
        // REQUIRE(archives_json == "");
        const auto archives_loaded = [&archives_json] {
            using Archives =
                decltype(immer::archive::detail::generate_archives_load(
                    get_archives_types(test_data{})));
            auto archives = Archives{};

            {
                auto is = std::istringstream{archives_json};
                auto ar = cereal::JSONInputArchive{is};
                ar(CEREAL_NVP(archives));
            }

            {
                auto is = std::istringstream{archives_json};
                auto ar = immer::archive::json_immer_input_archive<Archives>{
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
            immer::archive::from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer::archive::to_json_with_archive(full_load).first == "");
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
        immer::archive::to_json_with_archive(std::make_pair(test1, test2));

    // REQUIRE(json_str == "");

    {
        auto [loaded1, loaded2] = immer::archive::from_json_with_archive<
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
        immer::archive::to_json_with_archive(value).first;
    REQUIRE(json_archive_str == test::to_json(value));

    {
        auto loaded = immer::archive::from_json_with_archive<
            std::decay_t<decltype(value)>>(json_archive_str);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special archive loads empty test_data")
{
    const auto value = test_data{};

    // const auto json_archive_str =
    //     immer::archive::to_json_with_archive(value).first;
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
        auto loaded = immer::archive::from_json_with_archive<
            std::decay_t<decltype(value)>>(json_archive_str);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special archive throws cereal::Exception")
{
    const auto value = test_data{};

    // const auto json_archive_str =
    //     immer::archive::to_json_with_archive(value).first;
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
        immer::archive::from_json_with_archive<test_data>(json_archive_str),
        ::cereal::Exception,
        MessageMatches(Catch::Matchers::ContainsSubstring(
                           "Failed to load a container ID 99 from the "
                           "archive of immer::vector<int") &&
                       Catch::Matchers::ContainsSubstring(
                           "Container ID 99 is not found")));
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

    const auto [json_str, archives] = immer::archive::to_json_with_archive(v3);
    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer::archive::from_json_with_archive<recursive_type>(json_str);
        REQUIRE(full_load == v3);
        REQUIRE(immer::archive::to_json_with_archive(full_load).first ==
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

    const auto [json_str, archives] = immer::archive::to_json_with_archive(v5);
    // REQUIRE(json_str == "");

    {
        const auto full_load =
            immer::archive::from_json_with_archive<recursive_type>(json_str);
        REQUIRE(full_load == v5);
        REQUIRE(immer::archive::to_json_with_archive(full_load).first ==
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
        immer::archive::to_json_with_archive(value);
    // REQUIRE(json_str == "");

    {
        const auto full_load =
            immer::archive::from_json_with_archive<rec_map>(json_str);
        REQUIRE(full_load == value);
        REQUIRE(immer::archive::to_json_with_archive(full_load).first ==
                json_str);
    }
}

TEST_CASE("Test non-unique names in the map")
{
    {
        const auto names = hana::make_map(
            hana::make_pair(hana::type_c<immer::box<int>>,
                            BOOST_HANA_STRING("box_1")),
            hana::make_pair(hana::type_c<immer::box<std::string>>,
                            BOOST_HANA_STRING("box_2"))

        );

        using IsUnique =
            decltype(immer::archive::detail::are_type_names_unique(names));
        static_assert(IsUnique::value, "Names are unique");
    }
}

namespace {
using test::new_type;
using test::old_type;

template <class V>
using map_t = immer::map<std::string, V, immer::archive::xx_hash<std::string>>;

template <class T>
using table_t =
    immer::table<T, immer::table_key_fn, immer::archive::xx_hash<std::string>>;

// Some type that an application would serialize. Contains multiple vectors and
// maps to demonstrate structural sharing.
struct old_app_type
{
    arch<test::vector_one<old_type>> vec;
    arch<test::vector_one<old_type>> vec2;
    arch<map_t<old_type>> map;
    arch<map_t<old_type>> map2;
    arch<table_t<old_type>> table;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(vec),
           CEREAL_NVP(vec2),
           CEREAL_NVP(map),
           CEREAL_NVP(map2),
           CEREAL_NVP(table));
    }
};

auto get_archives_types(const old_app_type&)
{
    return hana::make_map(
        hana::make_pair(hana::type_c<test::vector_one<old_type>>,
                        BOOST_HANA_STRING("vec")),
        hana::make_pair(hana::type_c<map_t<old_type>>,
                        BOOST_HANA_STRING("map")),
        hana::make_pair(hana::type_c<table_t<old_type>>,
                        BOOST_HANA_STRING("table"))

    );
}

/**
 * We want to load and transform the old type into the new type.
 *
 * An approach to first load the old type and then apply some transformation
 * would not preserve the structural sharing within the type. (Converting 2
 * vectors that initially use structural sharing would result in 2 independent
 * vectors without SS).
 *
 * Therefore, we have to apply the transformation to the archives that are later
 * used to materialize these new vectors that would preserve SS.
 *
 * The new type can't differ much from the old type. The type's JSON layout must
 * be the same as the old type. Each archivable member gets serialized into an
 * integer (container ID within the archive), so that works. But we can't add
 * new members.
 */
struct new_app_type
{
    arch<test::vector_one<new_type>> vec;
    arch<test::vector_one<new_type>> vec2;
    arch<map_t<new_type>> map;
    arch<map_t<new_type>> map2;

    // Demonstrate the member that we do not upgrade.
    arch<table_t<old_type>> table;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(vec),
           CEREAL_NVP(vec2),
           CEREAL_NVP(map),
           CEREAL_NVP(map2),
           CEREAL_NVP(table));
    }
};

auto get_archives_types(const new_app_type&)
{
    return hana::make_map(
        hana::make_pair(hana::type_c<test::vector_one<new_type>>,
                        BOOST_HANA_STRING("vec")),
        hana::make_pair(hana::type_c<map_t<new_type>>,
                        BOOST_HANA_STRING("map")),
        hana::make_pair(hana::type_c<table_t<old_type>>,
                        BOOST_HANA_STRING("table"))

    );
}
} // namespace

TEST_CASE("Test conversion with a special archive")
{
    const auto vec1 = test::vector_one<old_type>{
        old_type{.data = 123},
        old_type{.data = 234},
    };
    const auto vec2 = vec1.push_back(old_type{.data = 345});

    const auto map1 = [] {
        auto map = map_t<old_type>{};
        for (auto i = 0; i < 30; ++i) {
            map =
                std::move(map).set(fmt::format("x{}x", i), old_type{.data = i});
        }
        return map;
    }();
    const auto map2 = map1.set("345", old_type{.data = 345});

    // Prepare a value of the old type that uses some structural sharing
    // internally.
    const auto value = old_app_type{
        .vec  = vec1,
        .vec2 = vec2,
        .map  = map1,
        .map2 = map2,
        .table =
            {
                old_type{"_51_", 51},
                old_type{"_52_", 52},
                old_type{"_53_", 53},
            },
    };
    const auto [json_str, archives] =
        immer::archive::to_json_with_archive(value);
    // REQUIRE(json_str == "");

    // Describe how to go from the old archive to the desired new archive.
    const auto archives_conversions =
        hana::make_map(hana::make_pair(
                           // Take this archive
                           hana::type_c<test::vector_one<old_type>>,
                           // And apply this conversion function to it
                           test::convert_old_type),
                       hana::make_pair(hana::type_c<map_t<old_type>>,
                                       test::convert_old_type_map)

        );

    // Having a JSON from serializing old_app_type and a conversion function,
    // we need to somehow load new_app_type.
    const new_app_type full_load =
        immer::archive::from_json_with_archive_with_conversion<new_app_type,
                                                               old_app_type>(
            json_str, archives_conversions);

    {
        REQUIRE(full_load.vec.container == transform_vec(value.vec.container));
        REQUIRE(full_load.vec2.container ==
                transform_vec(value.vec2.container));
        REQUIRE(full_load.map.container == transform_map(value.map.container));
        REQUIRE(full_load.map2.container ==
                transform_map(value.map2.container));
        REQUIRE(full_load.table.container == value.table.container);
    }

    SECTION(
        "Demonstrate that the loaded vectors and maps still share structure")
    {
        const auto [json_str, archives] =
            immer::archive::to_json_with_archive(full_load);
        // For example, "x21x" is stored only once.
        const auto expected = json_t::parse(R"(
{
  "archives": {
    "map": [
      {
        "children": [
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          9
        ],
        "collisions": false,
        "datamap": 2013603464,
        "nodemap": 2188935188,
        "values": [
          {
            "first": "x13x",
            "second": {
              "data": 13,
              "data2": "_13_",
              "id": ""
            }
          },
          {
            "first": "x4x",
            "second": {
              "data": 4,
              "data2": "_4_",
              "id": ""
            }
          },
          {
            "first": "x22x",
            "second": {
              "data": 22,
              "data2": "_22_",
              "id": ""
            }
          },
          {
            "first": "x28x",
            "second": {
              "data": 28,
              "data2": "_28_",
              "id": ""
            }
          },
          {
            "first": "x10x",
            "second": {
              "data": 10,
              "data2": "_10_",
              "id": ""
            }
          },
          {
            "first": "x12x",
            "second": {
              "data": 12,
              "data2": "_12_",
              "id": ""
            }
          },
          {
            "first": "x9x",
            "second": {
              "data": 9,
              "data2": "_9_",
              "id": ""
            }
          },
          {
            "first": "x29x",
            "second": {
              "data": 29,
              "data2": "_29_",
              "id": ""
            }
          },
          {
            "first": "x6x",
            "second": {
              "data": 6,
              "data2": "_6_",
              "id": ""
            }
          },
          {
            "first": "x17x",
            "second": {
              "data": 17,
              "data2": "_17_",
              "id": ""
            }
          },
          {
            "first": "x11x",
            "second": {
              "data": 11,
              "data2": "_11_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 67125248,
        "nodemap": 0,
        "values": [
          {
            "first": "x21x",
            "second": {
              "data": 21,
              "data2": "_21_",
              "id": ""
            }
          },
          {
            "first": "x5x",
            "second": {
              "data": 5,
              "data2": "_5_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 32770,
        "nodemap": 0,
        "values": [
          {
            "first": "x25x",
            "second": {
              "data": 25,
              "data2": "_25_",
              "id": ""
            }
          },
          {
            "first": "x26x",
            "second": {
              "data": 26,
              "data2": "_26_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 65539,
        "nodemap": 0,
        "values": [
          {
            "first": "x8x",
            "second": {
              "data": 8,
              "data2": "_8_",
              "id": ""
            }
          },
          {
            "first": "x16x",
            "second": {
              "data": 16,
              "data2": "_16_",
              "id": ""
            }
          },
          {
            "first": "x3x",
            "second": {
              "data": 3,
              "data2": "_3_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 139264,
        "nodemap": 0,
        "values": [
          {
            "first": "x14x",
            "second": {
              "data": 14,
              "data2": "_14_",
              "id": ""
            }
          },
          {
            "first": "x18x",
            "second": {
              "data": 18,
              "data2": "_18_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 1073742080,
        "nodemap": 0,
        "values": [
          {
            "first": "x23x",
            "second": {
              "data": 23,
              "data2": "_23_",
              "id": ""
            }
          },
          {
            "first": "x0x",
            "second": {
              "data": 0,
              "data2": "_0_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 2621440,
        "nodemap": 0,
        "values": [
          {
            "first": "x15x",
            "second": {
              "data": 15,
              "data2": "_15_",
              "id": ""
            }
          },
          {
            "first": "x24x",
            "second": {
              "data": 24,
              "data2": "_24_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 8224,
        "nodemap": 0,
        "values": [
          {
            "first": "x27x",
            "second": {
              "data": 27,
              "data2": "_27_",
              "id": ""
            }
          },
          {
            "first": "x1x",
            "second": {
              "data": 1,
              "data2": "_1_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 8421376,
        "nodemap": 0,
        "values": [
          {
            "first": "x7x",
            "second": {
              "data": 7,
              "data2": "_7_",
              "id": ""
            }
          },
          {
            "first": "x20x",
            "second": {
              "data": 20,
              "data2": "_20_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [],
        "collisions": false,
        "datamap": 134234112,
        "nodemap": 0,
        "values": [
          {
            "first": "x19x",
            "second": {
              "data": 19,
              "data2": "_19_",
              "id": ""
            }
          },
          {
            "first": "x2x",
            "second": {
              "data": 2,
              "data2": "_2_",
              "id": ""
            }
          }
        ]
      },
      {
        "children": [
          1,
          2,
          3,
          4,
          5,
          6,
          7,
          8,
          9
        ],
        "collisions": false,
        "datamap": 2013619848,
        "nodemap": 2188935188,
        "values": [
          {
            "first": "x13x",
            "second": {
              "data": 13,
              "data2": "_13_",
              "id": ""
            }
          },
          {
            "first": "x4x",
            "second": {
              "data": 4,
              "data2": "_4_",
              "id": ""
            }
          },
          {
            "first": "x22x",
            "second": {
              "data": 22,
              "data2": "_22_",
              "id": ""
            }
          },
          {
            "first": "x28x",
            "second": {
              "data": 28,
              "data2": "_28_",
              "id": ""
            }
          },
          {
            "first": "x10x",
            "second": {
              "data": 10,
              "data2": "_10_",
              "id": ""
            }
          },
          {
            "first": "x12x",
            "second": {
              "data": 12,
              "data2": "_12_",
              "id": ""
            }
          },
          {
            "first": "x9x",
            "second": {
              "data": 9,
              "data2": "_9_",
              "id": ""
            }
          },
          {
            "first": "x29x",
            "second": {
              "data": 29,
              "data2": "_29_",
              "id": ""
            }
          },
          {
            "first": "x6x",
            "second": {
              "data": 6,
              "data2": "_6_",
              "id": ""
            }
          },
          {
            "first": "345",
            "second": {
              "data": 345,
              "data2": "_345_",
              "id": ""
            }
          },
          {
            "first": "x17x",
            "second": {
              "data": 17,
              "data2": "_17_",
              "id": ""
            }
          },
          {
            "first": "x11x",
            "second": {
              "data": 11,
              "data2": "_11_",
              "id": ""
            }
          }
        ]
      }
    ],
    "table": [
      {
        "children": [],
        "collisions": false,
        "datamap": 1544,
        "nodemap": 0,
        "values": [
          {
            "data": 53,
            "id": "_53_"
          },
          {
            "data": 52,
            "id": "_52_"
          },
          {
            "data": 51,
            "id": "_51_"
          }
        ]
      }
    ],
    "vec": {
      "inners": [
        {
          "key": 0,
          "value": {
            "children": [],
            "relaxed": false
          }
        },
        {
          "key": 2,
          "value": {
            "children": [
              1
            ],
            "relaxed": false
          }
        }
      ],
      "leaves": [
        {
          "key": 1,
          "value": [
            {
              "data": 123,
              "data2": "_123_",
              "id": ""
            },
            {
              "data": 234,
              "data2": "_234_",
              "id": ""
            }
          ]
        },
        {
          "key": 3,
          "value": [
            {
              "data": 345,
              "data2": "_345_",
              "id": ""
            }
          ]
        }
      ],
      "vectors": [
        {
          "root": 0,
          "tail": 1
        },
        {
          "root": 2,
          "tail": 3
        }
      ]
    }
  },
  "value0": {
    "map": 0,
    "map2": 10,
    "table": 0,
    "vec": 0,
    "vec2": 1
  }
}
        )");
        REQUIRE(json_t::parse(json_str) == expected);
    }
}
