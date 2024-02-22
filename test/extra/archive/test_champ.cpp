#include <catch2/catch_test_macros.hpp>

#include <immer/extra/archive/champ/champ.hpp>
#include <immer/extra/archive/xxhash/xxhash.hpp>

#include "utils.hpp"

#include <nlohmann/json.hpp>

using namespace test;
using json_t = nlohmann::json;
using immer::archive::node_id;

namespace {

const auto gen_set = [](auto set, int count) {
    for (int i = 0; i < count; ++i) {
        set = std::move(set).insert(fmt::format("{}", i));
    }
    return set;
};

const auto gen_map = [](auto map, int count) {
    for (int i = 0; i < count; ++i) {
        map = std::move(map).set(i, fmt::format("_{}_", i));
    }
    return map;
};

auto gen_table(auto table, std::size_t from, std::size_t to)
{
    for (auto i = std::min(from, to); i < std::max(from, to); ++i) {
        table = std::move(table).insert(test_value{i, fmt::format("_{}_", i)});
    }
    return table;
}

const auto into_set = [](const auto& set) {
    using T     = typename std::decay_t<decltype(set)>::value_type;
    auto result = immer::set<T>{};
    for (const auto& item : set) {
        result = std::move(result).insert(item);
    }
    return result;
};

const auto into_map = [](const auto& map) {
    using K     = typename std::decay_t<decltype(map)>::key_type;
    using V     = typename std::decay_t<decltype(map)>::mapped_type;
    auto result = immer::map<K, V>{};
    for (const auto& item : map) {
        result = std::move(result).insert(item);
    }
    return result;
};

struct broken_hash
{
    std::size_t operator()(const std::string& str) const
    {
        if ("10" < str && str < "19") {
            return 123;
        }
        return immer::archive::xx_hash<std::string>{}(str);
    }

    std::size_t operator()(int map_key) const
    {
        if (10 < map_key && map_key < 19) {
            return 123;
        }
        return std::hash<int>{}(map_key);
    }
};

} // namespace

TEST_CASE("Test saving a set")
{
    using Container = immer::set<std::string, broken_hash>;

    const auto set          = gen_set(Container{}, 200);
    const auto [ar, set_id] = immer::archive::champ::save_to_archive(set, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_archive =
            from_json<immer::archive::champ::container_archive_load<Container>>(
                ar_str);

        auto loader = immer::archive::champ::container_loader{loaded_archive};
        const auto loaded = loader.load(set_id);
        REQUIRE(into_set(set).size() == set.size());
        for (const auto& item : set) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
    }

    SECTION("Load with a different hash")
    {
        using WrongContainer      = immer::set<std::string>;
        const auto loaded_archive = from_json<
            immer::archive::champ::container_archive_load<WrongContainer>>(
            ar_str);

        auto loader = immer::archive::champ::container_loader{loaded_archive};
        REQUIRE_THROWS_AS(
            loader.load(set_id),
            immer::archive::champ::hash_validation_failed_exception);
    }
}

TEST_CASE("Test set with xxHash")
{
    using Container =
        immer::set<std::string, immer::archive::xx_hash<std::string>>;

    const auto set          = gen_set(Container{}, 200);
    const auto [ar, set_id] = immer::archive::champ::save_to_archive(set, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_archive =
            from_json<immer::archive::champ::container_archive_load<Container>>(
                ar_str);

        auto loader = immer::archive::champ::container_loader{loaded_archive};
        const auto loaded = loader.load(set_id);
        REQUIRE(into_set(set).size() == set.size());
        for (const auto& item : set) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
    }
}

TEST_CASE("Test archive conversion, no json")
{
    using Container = immer::set<std::string, broken_hash>;

    const auto set        = gen_set(Container{}, 200);
    const auto set2       = gen_set(set, 300);
    auto [ar, set_id]     = immer::archive::champ::save_to_archive(set, {});
    auto set2_id          = immer::archive::node_id{};
    std::tie(ar, set2_id) = immer::archive::champ::save_to_archive(set2, ar);

    auto loader = immer::archive::champ::container_loader{to_load_archive(ar)};

    const auto check_set = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        REQUIRE(into_set(loaded) == into_set(expected));
        REQUIRE(into_set(expected).size() == expected.size());
        for (const auto& item : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
        for (const auto& item : loaded) {
            REQUIRE(expected.count(item));
        }
    };

    check_set(set_id, set);
    check_set(set2_id, set2);
}

TEST_CASE("Test save mutated set")
{
    using Container = immer::set<std::string, broken_hash>;

    auto set          = gen_set(Container{}, 200);
    auto [ar, set_id] = immer::archive::champ::save_to_archive(set, {});

    set                   = std::move(set).insert("435");
    auto set_id2          = immer::archive::node_id{};
    std::tie(ar, set_id2) = immer::archive::champ::save_to_archive(set, ar);

    REQUIRE(set_id != set_id2);
}

TEST_CASE("Test saving a map")
{
    using Container = immer::map<int, std::string, broken_hash>;

    const auto map          = gen_map(Container{}, 200);
    const auto [ar, map_id] = immer::archive::champ::save_to_archive(map, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_archive =
            from_json<immer::archive::champ::container_archive_load<Container>>(
                ar_str);

        auto loader = immer::archive::champ::container_loader{loaded_archive};
        const auto loaded = loader.load(map_id);
        REQUIRE(into_map(map).size() == map.size());
        for (const auto& [key, value] : map) {
            // This is the only thing that actually breaks if the hash of the
            // loaded map is not the same as the hash function of the serialized
            // map.
            REQUIRE(loaded.count(key));
            REQUIRE(loaded[key] == value);
        }
    }

    SECTION("Load with a different hash")
    {
        using WrongContainer      = immer::map<int, std::string>;
        const auto loaded_archive = from_json<
            immer::archive::champ::container_archive_load<WrongContainer>>(
            ar_str);

        auto loader = immer::archive::champ::container_loader{loaded_archive};
        REQUIRE_THROWS_AS(
            loader.load(map_id),
            immer::archive::champ::hash_validation_failed_exception);
    }
}

TEST_CASE("Test map archive conversion, no json")
{
    using Container = immer::map<int, std::string, broken_hash>;

    const auto map        = gen_map(Container{}, 200);
    const auto map2       = gen_map(map, 300);
    auto [ar, map_id]     = immer::archive::champ::save_to_archive(map, {});
    auto map2_id          = immer::archive::node_id{};
    std::tie(ar, map2_id) = immer::archive::champ::save_to_archive(map2, ar);

    auto loader = immer::archive::champ::container_loader{to_load_archive(ar)};

    const auto check_map = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        REQUIRE(into_map(loaded) == into_map(expected));
        REQUIRE(into_map(expected).size() == expected.size());
        for (const auto& [key, value] : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded map is not the same as the hash function of the serialized
            // map.
            REQUIRE(loaded.count(key));
            REQUIRE(loaded[key] == value);
        }
        for (const auto& [key, value] : loaded) {
            REQUIRE(expected.count(key));
            REQUIRE(expected[key] == value);
        }
    };

    check_map(map_id, map);
    check_map(map2_id, map2);
}

TEST_CASE("Test save mutated map")
{
    auto map = gen_map(immer::map<int, std::string, broken_hash>{}, 200);
    auto [ar, map_id] = immer::archive::champ::save_to_archive(map, {});

    map                   = std::move(map).set(999, "435");
    auto map_id2          = immer::archive::node_id{};
    std::tie(ar, map_id2) = immer::archive::champ::save_to_archive(map, ar);

    REQUIRE(map_id != map_id2);
}

namespace {
template <class T1, class T2, class Verify>
void test_table_types(Verify&& verify)
{
    const auto t1 = gen_table(T1{}, 0, 100);
    const auto t2 = gen_table(t1, 200, 210);

    auto [ar, t1_id] = immer::archive::champ::save_to_archive(t1, {});

    auto t2_id          = immer::archive::node_id{};
    std::tie(ar, t2_id) = immer::archive::champ::save_to_archive(t2, ar);

    const auto ar_str = to_json(ar);
    // REQUIRE(ar_str == "");

    const auto loaded_archive =
        from_json<immer::archive::champ::container_archive_load<T2>>(ar_str);

    auto loader = immer::archive::champ::container_loader{loaded_archive};

    const auto check = [&loader, &verify](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        verify(loaded, expected);
    };

    check(t1_id, t1);
    check(t2_id, t2);
}
} // namespace

TEST_CASE("Test saving a table")
{
    using different_table_t =
        immer::table<test_value, immer::table_key_fn, broken_hash>;
    using table_t = immer::table<test_value>;

    const auto verify_is_equal = [](const auto& loaded, const auto& expected) {
        for (const auto& item : expected) {
            INFO(item);
            REQUIRE(loaded.count(item.id));
            REQUIRE(loaded[item.id] == item);
        }
        for (const auto& item : loaded) {
            REQUIRE(expected[item.id] == item);
        }
    };

    // Verify that saving and loading with customized hash works.
    test_table_types<table_t, table_t>(verify_is_equal);
    test_table_types<different_table_t, different_table_t>(verify_is_equal);

    const auto test1 = [&] {
        test_table_types<different_table_t, table_t>(verify_is_equal);
    };
    const auto test2 = [&] {
        test_table_types<table_t, different_table_t>(verify_is_equal);
    };

    REQUIRE_THROWS_AS(test1(),
                      immer::archive::champ::hash_validation_failed_exception);
    REQUIRE_THROWS_AS(test2(),
                      immer::archive::champ::hash_validation_failed_exception);
}

TEST_CASE("Test saving a table, no json")
{
    using table_t = immer::table<test_value>;
    const auto t1 = gen_table(table_t{}, 0, 100);
    const auto t2 = gen_table(t1, 200, 210);

    auto [ar, t1_id] = immer::archive::champ::save_to_archive(t1, {});

    auto t2_id          = immer::archive::node_id{};
    std::tie(ar, t2_id) = immer::archive::champ::save_to_archive(t2, ar);

    const auto ar_str = to_json(ar);
    // REQUIRE(ar_str == "");

    auto loader = immer::archive::champ::container_loader{to_load_archive(ar)};

    const auto check = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);

        for (const auto& item : expected) {
            REQUIRE(loaded[item.id] == item);
        }
        for (const auto& item : loaded) {
            REQUIRE(expected[item.id] == item);
        }
    };

    check(t1_id, t1);
    check(t2_id, t2);
}

TEST_CASE("Test modifying set nodes")
{
    using Container =
        immer::set<std::string, immer::archive::xx_hash<std::string>>;

    const auto expected_set  = gen_set(Container{}, 30);
    const auto expected_set2 = expected_set.insert("thirty");

    auto [ar, set_id] =
        immer::archive::champ::save_to_archive(expected_set, {});
    auto set2_id = immer::archive::node_id{};
    std::tie(ar, set2_id) =
        immer::archive::champ::save_to_archive(expected_set2, std::move(ar));
    const auto ar_str        = to_json(ar);
    const auto expected_data = json_t::parse(ar_str);
    // REQUIRE(ar_str == "");

    auto data = json_t::parse(R"({
  "value0": [
      {
        "values": ["15", "27", "6", "0", "1", "20", "4"],
        "children": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
        "nodemap": 1343560972,
        "datamap": 19407009,
        "collisions": false
      },
      {
        "values": ["25", "24"],
        "children": [],
        "nodemap": 0,
        "datamap": 536875008,
        "collisions": false
      },
      {
        "values": ["14", "16"],
        "children": [],
        "nodemap": 0,
        "datamap": 16777224,
        "collisions": false
      },
      {
        "values": ["23", "26", "9"],
        "children": [],
        "nodemap": 0,
        "datamap": 2147747840,
        "collisions": false
      },
      {
        "values": ["13", "8"],
        "children": [],
        "nodemap": 0,
        "datamap": 16512,
        "collisions": false
      },
      {
        "values": ["28", "22"],
        "children": [],
        "nodemap": 0,
        "datamap": 8388640,
        "collisions": false
      },
      {
        "values": ["3", "19"],
        "children": [],
        "nodemap": 0,
        "datamap": 2147485696,
        "collisions": false
      },
      {
        "values": ["10", "5"],
        "children": [],
        "nodemap": 0,
        "datamap": 33556480,
        "collisions": false
      },
      {
        "values": ["29", "18"],
        "children": [],
        "nodemap": 0,
        "datamap": 16778240,
        "collisions": false
      },
      {
        "values": ["2", "12"],
        "children": [],
        "nodemap": 0,
        "datamap": 9,
        "collisions": false
      },
      {
        "values": ["7", "11"],
        "children": [],
        "nodemap": 0,
        "datamap": 8388616,
        "collisions": false
      },
      {
        "values": ["21", "17"],
        "children": [],
        "nodemap": 0,
        "datamap": 268451840,
        "collisions": false
      },
      {
        "values": ["15", "27", "6", "0", "1", "thirty", "20", "4"],
        "children": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
        "nodemap": 1343560972,
        "datamap": 19407011,
        "collisions": false
      }
    ]
})");
    REQUIRE(data == expected_data);

    const auto load_set = [&data](auto id) {
        const auto loaded_archive =
            from_json<immer::archive::champ::container_archive_load<Container>>(
                data.dump());
        auto loader = immer::archive::champ::container_loader{loaded_archive};
        return loader.load(id);
    };

    SECTION("Loads correctly")
    {
        REQUIRE(load_set(set_id) == expected_set);
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Modify nodemap of node 0")
    {
        auto& nodemap = data["value0"][0]["nodemap"];
        REQUIRE(nodemap == 1343560972);
        nodemap = 1343560971;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::archive::champ::children_count_corrupted_exception);
        // Node 0 doesn't affect the second set
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Modify datamap of node 0")
    {
        auto& datamap = data["value0"][0]["datamap"];
        REQUIRE(datamap == 19407009);
        datamap = 19407008;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::archive::champ::data_count_corrupted_exception);
        // Node 0 doesn't affect the second set
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Modify nodemap of node 1")
    {
        auto& nodemap = data["value0"][1]["nodemap"];
        REQUIRE(nodemap == 0);
        nodemap = 1;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::archive::champ::children_count_corrupted_exception);
        REQUIRE_THROWS_AS(
            load_set(set2_id),
            immer::archive::champ::children_count_corrupted_exception);
    }
    SECTION("Modify datamap of node 1")
    {
        auto& datamap = data["value0"][1]["datamap"];
        REQUIRE(datamap == 536875008);
        datamap = 536875007;
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::archive::champ::data_count_corrupted_exception);
        REQUIRE_THROWS_AS(
            load_set(set2_id),
            immer::archive::champ::data_count_corrupted_exception);
    }
    SECTION("Corrupt datamap but keep the same popcount")
    {
        auto& datamap = data["value0"][2]["datamap"];
        REQUIRE(datamap == 16777224);
        datamap = 536875008; // This number also has 2 bits set
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::archive::champ::hash_validation_failed_exception);
        REQUIRE_THROWS_AS(
            load_set(set2_id),
            immer::archive::champ::hash_validation_failed_exception);
    }
    SECTION("Corrupt nodemap but keep the same popcount")
    {
        auto& nodemap = data["value0"][0]["nodemap"];
        REQUIRE(nodemap == 1343560972);
        nodemap = 1343560460; // This number has the same number of bits set
        REQUIRE_THROWS_AS(
            load_set(set_id),
            immer::archive::champ::hash_validation_failed_exception);
        // Node 0 doesn't affect the second set
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Missing a child node")
    {
        auto& children = data["value0"][0]["children"];
        children       = {1, 2, 99, 4, 5, 6, 7, 8, 9, 10, 11};
        REQUIRE_THROWS_AS(load_set(set_id), immer::archive::invalid_node_id);
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Same identity")
    {
        // Have to keep the loader alive between loads, otherwise there's no way
        // to share the nodes.
        const auto loaded_archive =
            from_json<immer::archive::champ::container_archive_load<Container>>(
                data.dump());
        auto loader = immer::archive::champ::container_loader{loaded_archive};
        REQUIRE(loader.load(node_id{0}).identity() ==
                loader.load(node_id{0}).identity());
    }
}

TEST_CASE("Test modifying nodes with collisions")
{
    using Container = immer::set<std::string, broken_hash>;

    const auto expected_set  = gen_set(Container{}, 30);
    const auto expected_set2 = expected_set.insert("thirty");

    auto [ar, set_id] =
        immer::archive::champ::save_to_archive(expected_set, {});
    auto set2_id = immer::archive::node_id{};
    std::tie(ar, set2_id) =
        immer::archive::champ::save_to_archive(expected_set2, std::move(ar));
    const auto ar_str        = to_json(ar);
    const auto expected_data = json_t::parse(ar_str);

    auto data = json_t::parse(R"({
  "value0": [
    {
      "values": ["8", "27", "6", "29", "2", "0", "1", "7", "20", "4"],
      "children": [1, 2, 3, 4, 5, 6],
      "nodemap": 269550856,
      "datamap": 2898085,
      "collisions": false
    },
    {
      "values": ["25", "24"],
      "children": [],
      "nodemap": 0,
      "datamap": 536875008,
      "collisions": false
    },
    {
      "values": ["23", "26", "9"],
      "children": [],
      "nodemap": 0,
      "datamap": 2147747840,
      "collisions": false
    },
    {
      "values": ["28", "22"],
      "children": [],
      "nodemap": 0,
      "datamap": 8388640,
      "collisions": false
    },
    {
      "values": ["3", "19"],
      "children": [],
      "nodemap": 0,
      "datamap": 2147485696,
      "collisions": false
    },
    {
      "values": ["10", "5"],
      "children": [],
      "nodemap": 0,
      "datamap": 33556480,
      "collisions": false
    },
    {
      "values": ["21"],
      "children": [7],
      "nodemap": 134217728,
      "datamap": 268435456,
      "collisions": false
    },
    {
      "values": [],
      "children": [8],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [9],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [10],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [11],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [12],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [13],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [14],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [15],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [16],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [17],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": [],
      "children": [18],
      "nodemap": 16777216,
      "datamap": 0,
      "collisions": false
    },
    {
      "values": ["18", "17", "16", "15", "14", "13", "12", "11"],
      "children": [],
      "nodemap": 0,
      "datamap": 0,
      "collisions": true
    },
    {
      "values": ["8", "27", "6", "29", "2", "0", "1", "thirty", "7", "20", "4"],
      "children": [1, 2, 3, 4, 5, 6],
      "nodemap": 269550856,
      "datamap": 2898087,
      "collisions": false
    }
  ]
})");
    INFO(ar_str);
    REQUIRE(data == expected_data);

    const auto load_set = [&data](auto id) {
        const auto loaded_archive =
            from_json<immer::archive::champ::container_archive_load<Container>>(
                data.dump());
        auto loader = immer::archive::champ::container_loader{loaded_archive};
        return loader.load(id);
    };

    SECTION("Loads correctly")
    {
        REQUIRE(load_set(set_id) == expected_set);
        REQUIRE(load_set(set2_id) == expected_set2);
    }
    SECTION("Order of collisions doesn't matter")
    {
        auto& node = data["value0"][18];
        REQUIRE(
            node["values"] ==
            json_t::array({"18", "17", "16", "15", "14", "13", "12", "11"}));
        node["values"] = {"15", "16", "17", "18", "14", "13", "12", "11"};
        REQUIRE(load_set(set_id) == expected_set);
        REQUIRE(load_set(set2_id) == expected_set2);

        // Remove "18"
        node["values"] = {"15", "16", "17", "14", "13", "12", "11"};
        REQUIRE(load_set(set_id) == expected_set.erase("18"));
    }
}

TEST_CASE("Test champ archive conversion")
{
    using test::new_type;
    using test::old_type;

    using old_map_t =
        immer::map<std::string, old_type, immer::archive::xx_hash<std::string>>;
    using new_map_t =
        immer::map<std::string, new_type, immer::archive::xx_hash<std::string>>;

    const auto map1 = [] {
        auto map = old_map_t{};
        for (auto i = 0; i < 30; ++i) {
            map = std::move(map).set(fmt::format("x{}x", i), old_type{i});
        }
        return map;
    }();
    const auto map2 = map1.set("345", old_type{345});

    auto [ar, map1_id]    = immer::archive::champ::save_to_archive(map1, {});
    auto map2_id          = immer::archive::node_id{};
    std::tie(ar, map2_id) = save_to_archive(map2, ar);

    // Confirm that map1 and map2 have structural sharing in the beginning.
    // For example, "x21x" is stored only once, it's shared.
    const auto expected_ar = json_t::parse(R"(
{
  "value0": [
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
            "data": 13
          }
        },
        {
          "first": "x4x",
          "second": {
            "data": 4
          }
        },
        {
          "first": "x22x",
          "second": {
            "data": 22
          }
        },
        {
          "first": "x28x",
          "second": {
            "data": 28
          }
        },
        {
          "first": "x10x",
          "second": {
            "data": 10
          }
        },
        {
          "first": "x12x",
          "second": {
            "data": 12
          }
        },
        {
          "first": "x9x",
          "second": {
            "data": 9
          }
        },
        {
          "first": "x29x",
          "second": {
            "data": 29
          }
        },
        {
          "first": "x6x",
          "second": {
            "data": 6
          }
        },
        {
          "first": "x17x",
          "second": {
            "data": 17
          }
        },
        {
          "first": "x11x",
          "second": {
            "data": 11
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
            "data": 21
          }
        },
        {
          "first": "x5x",
          "second": {
            "data": 5
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
            "data": 25
          }
        },
        {
          "first": "x26x",
          "second": {
            "data": 26
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
            "data": 8
          }
        },
        {
          "first": "x16x",
          "second": {
            "data": 16
          }
        },
        {
          "first": "x3x",
          "second": {
            "data": 3
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
            "data": 14
          }
        },
        {
          "first": "x18x",
          "second": {
            "data": 18
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
            "data": 23
          }
        },
        {
          "first": "x0x",
          "second": {
            "data": 0
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
            "data": 15
          }
        },
        {
          "first": "x24x",
          "second": {
            "data": 24
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
            "data": 27
          }
        },
        {
          "first": "x1x",
          "second": {
            "data": 1
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
            "data": 7
          }
        },
        {
          "first": "x20x",
          "second": {
            "data": 20
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
            "data": 19
          }
        },
        {
          "first": "x2x",
          "second": {
            "data": 2
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
            "data": 13
          }
        },
        {
          "first": "x4x",
          "second": {
            "data": 4
          }
        },
        {
          "first": "x22x",
          "second": {
            "data": 22
          }
        },
        {
          "first": "x28x",
          "second": {
            "data": 28
          }
        },
        {
          "first": "x10x",
          "second": {
            "data": 10
          }
        },
        {
          "first": "x12x",
          "second": {
            "data": 12
          }
        },
        {
          "first": "x9x",
          "second": {
            "data": 9
          }
        },
        {
          "first": "x29x",
          "second": {
            "data": 29
          }
        },
        {
          "first": "x6x",
          "second": {
            "data": 6
          }
        },
        {
          "first": "345",
          "second": {
            "data": 345
          }
        },
        {
          "first": "x17x",
          "second": {
            "data": 17
          }
        },
        {
          "first": "x11x",
          "second": {
            "data": 11
          }
        }
      ]
    }
  ]
}
    )");

    REQUIRE(json_t::parse(to_json(ar)) == expected_ar);

    const auto transform_map = [&](const auto& map) {
        auto result = new_map_t{};
        for (const auto& [key, value] : map) {
            result = std::move(result).set(key, convert_old_type(value));
        }
        return result;
    };

    const auto load_archive = to_load_archive(ar);
    const auto load_archive_new_type =
        transform_archive(load_archive, convert_old_type);
    auto loader =
        immer::archive::champ::container_loader{load_archive_new_type};

    const auto loaded_1 = loader.load(map1_id);
    const auto loaded_2 = loader.load(map2_id);
    REQUIRE(loaded_1 == transform_map(map1));
    REQUIRE(loaded_2 == transform_map(map2));

    SECTION("Loaded maps still share the structure")
    {
        auto [ar, id]    = immer::archive::champ::save_to_archive(loaded_1, {});
        std::tie(ar, id) = save_to_archive(loaded_1, ar);
        std::tie(ar, id) = save_to_archive(loaded_2, ar);

        // "_21_" is stored only once
        const auto expected_ar = json_t::parse(R"(
{
  "value0": [
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
            "data2": "_13_"
          }
        },
        {
          "first": "x4x",
          "second": {
            "data": 4,
            "data2": "_4_"
          }
        },
        {
          "first": "x22x",
          "second": {
            "data": 22,
            "data2": "_22_"
          }
        },
        {
          "first": "x28x",
          "second": {
            "data": 28,
            "data2": "_28_"
          }
        },
        {
          "first": "x10x",
          "second": {
            "data": 10,
            "data2": "_10_"
          }
        },
        {
          "first": "x12x",
          "second": {
            "data": 12,
            "data2": "_12_"
          }
        },
        {
          "first": "x9x",
          "second": {
            "data": 9,
            "data2": "_9_"
          }
        },
        {
          "first": "x29x",
          "second": {
            "data": 29,
            "data2": "_29_"
          }
        },
        {
          "first": "x6x",
          "second": {
            "data": 6,
            "data2": "_6_"
          }
        },
        {
          "first": "x17x",
          "second": {
            "data": 17,
            "data2": "_17_"
          }
        },
        {
          "first": "x11x",
          "second": {
            "data": 11,
            "data2": "_11_"
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
            "data2": "_21_"
          }
        },
        {
          "first": "x5x",
          "second": {
            "data": 5,
            "data2": "_5_"
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
            "data2": "_25_"
          }
        },
        {
          "first": "x26x",
          "second": {
            "data": 26,
            "data2": "_26_"
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
            "data2": "_8_"
          }
        },
        {
          "first": "x16x",
          "second": {
            "data": 16,
            "data2": "_16_"
          }
        },
        {
          "first": "x3x",
          "second": {
            "data": 3,
            "data2": "_3_"
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
            "data2": "_14_"
          }
        },
        {
          "first": "x18x",
          "second": {
            "data": 18,
            "data2": "_18_"
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
            "data2": "_23_"
          }
        },
        {
          "first": "x0x",
          "second": {
            "data": 0,
            "data2": "_0_"
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
            "data2": "_15_"
          }
        },
        {
          "first": "x24x",
          "second": {
            "data": 24,
            "data2": "_24_"
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
            "data2": "_27_"
          }
        },
        {
          "first": "x1x",
          "second": {
            "data": 1,
            "data2": "_1_"
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
            "data2": "_7_"
          }
        },
        {
          "first": "x20x",
          "second": {
            "data": 20,
            "data2": "_20_"
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
            "data2": "_19_"
          }
        },
        {
          "first": "x2x",
          "second": {
            "data": 2,
            "data2": "_2_"
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
            "data2": "_13_"
          }
        },
        {
          "first": "x4x",
          "second": {
            "data": 4,
            "data2": "_4_"
          }
        },
        {
          "first": "x22x",
          "second": {
            "data": 22,
            "data2": "_22_"
          }
        },
        {
          "first": "x28x",
          "second": {
            "data": 28,
            "data2": "_28_"
          }
        },
        {
          "first": "x10x",
          "second": {
            "data": 10,
            "data2": "_10_"
          }
        },
        {
          "first": "x12x",
          "second": {
            "data": 12,
            "data2": "_12_"
          }
        },
        {
          "first": "x9x",
          "second": {
            "data": 9,
            "data2": "_9_"
          }
        },
        {
          "first": "x29x",
          "second": {
            "data": 29,
            "data2": "_29_"
          }
        },
        {
          "first": "x6x",
          "second": {
            "data": 6,
            "data2": "_6_"
          }
        },
        {
          "first": "345",
          "second": {
            "data": 345,
            "data2": "_345_"
          }
        },
        {
          "first": "x17x",
          "second": {
            "data": 17,
            "data2": "_17_"
          }
        },
        {
          "first": "x11x",
          "second": {
            "data": 11,
            "data2": "_11_"
          }
        }
      ]
    }
  ]
}
        )");
        REQUIRE(json_t::parse(to_json(ar)) == expected_ar);
    }
}
