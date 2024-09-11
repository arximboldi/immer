//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <immer/extra/persist/detail/rbts/input.hpp>
#include <immer/extra/persist/detail/rbts/output.hpp>

#include "utils.hpp"

#include <boost/hana.hpp>
#include <boost/hana/ext/std/tuple.hpp>

#include <nlohmann/json.hpp>

namespace {

using namespace test;
using immer::persist::container_id;
using immer::persist::rbts::add_to_pool;
using json_t   = nlohmann::json;
namespace hana = boost::hana;

template <class T>
auto load_vec(const T& json, std::size_t vec_id)
{
    const auto pool =
        test::from_json<immer::persist::rbts::input_pool<int>>(json);
    auto loader =
        immer::persist::rbts::make_loader_for(test::example_vector{}, pool);
    return loader.load(container_id{vec_id});
}

template <class T>
auto load_flex_vec(const T& json, std::size_t vec_id)
{
    const auto pool =
        test::from_json<immer::persist::rbts::input_pool<int>>(json);
    auto loader = immer::persist::rbts::make_loader_for(
        test::example_flex_vector{}, pool);
    return loader.load(container_id{vec_id});
}

} // namespace

TEST_CASE("Save and load multiple times into the same pool")
{
    auto test_vectors = std::vector<example_vector>{
        // gen(example_vector{}, 4)
        example_vector{},
    };
    auto counter = std::size_t{};
    auto pool    = immer::persist::rbts::make_output_pool_for(example_vector{});
    const auto save_and_load = [&]() {
        const auto vec = test_vectors.back().push_back(++counter);
        test_vectors.push_back(vec);

        auto vector_id = immer::persist::container_id{};
        std::tie(pool, vector_id) =
            immer::persist::rbts::add_to_pool(vec, pool);

        {
            auto loader =
                std::make_optional(example_loader{to_input_pool(pool)});
            auto loaded_vec = loader->load_vector(vector_id);
            REQUIRE(loaded_vec == vec);
        }
    };
    // Memory leak investigation: starts only with 4 iterations.
    // for (int i = 0; i < 4; ++i) {
    //     save_and_load();
    // }
    save_and_load();
    save_and_load();
    save_and_load();
    save_and_load();
}

TEST_CASE("Save and load vectors with shared nodes")
{
    // Create a bunch of vectors with shared nodes
    const auto generate_vectors = [] {
        const auto v1 = gen(example_vector{}, 69);
        const auto v2 = v1.push_back(900);
        const auto v3 = v2.push_back(901);
        return std::vector<example_vector>{v1, v2, v3};
    };

    // Save them
    const auto save_vectors = [](const auto& vectors)
        -> std::pair<example_output_pool,
                     std::vector<immer::persist::container_id>> {
        auto pool = example_output_pool{};
        auto ids  = std::vector<immer::persist::container_id>{};
        for (const auto& v : vectors) {
            auto [ar2, id] = add_to_pool(v, pool);
            pool           = ar2;
            ids.push_back(id);
        }
        REQUIRE(ids.size() == vectors.size());
        return {std::move(pool), std::move(ids)};
    };

    const auto vectors     = generate_vectors();
    const auto [pool, ids] = save_vectors(vectors);

    {
        // Check that if we generate the same but independent vectors and save
        // them again, the pools look the same
        const auto [ar2, ids2] = save_vectors(generate_vectors());
        REQUIRE(to_json(pool) == to_json(ar2));
    }

    // Load them and verify they're equal to the original vectors
    auto loader = std::make_optional(example_loader{to_input_pool(pool)});
    std::vector<example_vector> loaded;
    auto index = std::size_t{};
    for (const auto& id : ids) {
        auto v = loader->load_vector(id);
        REQUIRE(v == vectors[index]);
        loaded.push_back(v);
        ++index;
    }

    REQUIRE(loaded == vectors);

    SECTION("Deallocate loaded first, loader should collect all nodes")
    {
        loaded = {};
        // Now the loader should deallocate all the nodes it has.
    }
    SECTION("Deallocate loader first, vectors should collect their nodes")
    {
        loader.reset();
    }
}

TEST_CASE("Save and load vectors and flex vectors with shared nodes")
{
    // Create a bunch of vectors with shared nodes
    const auto generate_vectors = [] {
        const auto v1 = gen(example_vector{}, 69);
        const auto v2 = v1.push_back(900);
        const auto v3 = v2.push_back(901);
        return std::vector<example_vector>{
            v1,
            v2,
            v3,
        };
    };

    const auto generate_flex_vectors = [] {
        const auto v1 = gen(example_vector{}, 69);
        const auto v2 = example_flex_vector{v1}.push_back(900);
        const auto v3 = v2.push_back(901);
        return std::vector<example_flex_vector>{
            v1,
            v2,
            v3,
            v1 + v3,
            v3 + v2,
            v2 + v3 + v1,
        };
    };

    // Save them
    const auto save_vectors = [](example_output_pool pool, const auto& vectors)
        -> std::pair<example_output_pool,
                     std::vector<immer::persist::container_id>> {
        auto ids = std::vector<immer::persist::container_id>{};
        for (const auto& v : vectors) {
            auto [ar2, id] = add_to_pool(v, pool);
            pool           = ar2;
            ids.push_back(id);
        }
        REQUIRE(ids.size() == vectors.size());
        return {std::move(pool), std::move(ids)};
    };

    auto pool                  = example_output_pool{};
    const auto vectors         = generate_vectors();
    const auto flex_vectors    = generate_flex_vectors();
    auto vector_ids            = std::vector<immer::persist::container_id>{};
    auto flex_vectors_ids      = std::vector<immer::persist::container_id>{};
    std::tie(pool, vector_ids) = save_vectors(pool, vectors);
    std::tie(pool, flex_vectors_ids) = save_vectors(pool, flex_vectors);
    REQUIRE(!vector_ids.empty());
    REQUIRE(!flex_vectors_ids.empty());

    {
        // Check that if we generate the same but independent vectors and save
        // them again, the pools look the same
        const auto ar2 =
            save_vectors(save_vectors({}, generate_vectors()).first,
                         generate_flex_vectors())
                .first;
        REQUIRE(to_json(pool) == to_json(ar2));
    }

    // Load them and verify they're equal to the original vectors
    auto loader = example_loader{to_input_pool(pool)};
    auto loaded = [&] {
        auto result = std::vector<example_vector>{};
        auto index  = std::size_t{};
        for (const auto& id : vector_ids) {
            auto v = loader.load_vector(id);
            REQUIRE(v == vectors[index]);
            result.push_back(v);
            ++index;
        }
        return result;
    }();
    REQUIRE(loaded == vectors);

    auto loaded_flex = [&] {
        auto result = std::vector<example_flex_vector>{};
        auto index  = std::size_t{};
        for (const auto& id : flex_vectors_ids) {
            auto v = loader.load_flex_vector(id);
            REQUIRE(v == flex_vectors[index]);
            result.push_back(v);
            ++index;
        }
        return result;
    }();
    REQUIRE(loaded_flex == flex_vectors);

    loaded = {};
    // Now the loader should deallocated all the nodes it has.
}

TEST_CASE("Persist in-place mutated vector")
{
    auto vec            = example_vector{1, 2, 3};
    auto pool           = example_output_pool{};
    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = add_to_pool(vec, pool);

    vec                 = std::move(vec).push_back(90);
    auto id2            = immer::persist::container_id{};
    std::tie(pool, id2) = add_to_pool(vec, pool);

    REQUIRE(id1 != id2);

    auto loader        = example_loader{to_input_pool(pool)};
    const auto loaded1 = loader.load_vector(id1);
    const auto loaded2 = loader.load_vector(id2);
    REQUIRE(loaded2 == loaded1.push_back(90));
}

TEST_CASE("Persist in-place mutated flex_vector")
{
    auto vec            = example_flex_vector{1, 2, 3};
    auto pool           = example_output_pool{};
    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = add_to_pool(vec, pool);

    vec                 = std::move(vec).push_back(90);
    auto id2            = immer::persist::container_id{};
    std::tie(pool, id2) = add_to_pool(vec, pool);

    REQUIRE(id1 != id2);

    auto loader        = example_loader{to_input_pool(pool)};
    const auto loaded1 = loader.load_flex_vector(id1);
    const auto loaded2 = loader.load_flex_vector(id2);
    REQUIRE(loaded2 == loaded1.push_back(90));
}

TEST_CASE("Test nodes reuse")
{
    const auto small_vec = gen(test::flex_vector_one<int>{}, 67);
    const auto big_vec   = small_vec + small_vec;

    auto pool           = example_output_pool{};
    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = add_to_pool(big_vec, pool);

    {
        // Loads correctly
        auto loader        = example_loader{to_input_pool(pool)};
        const auto loaded1 = loader.load_flex_vector(id1);
        REQUIRE(loaded1 == big_vec);
    }

    // REQUIRE(to_json(pool) == "");
}

TEST_CASE("Test saving and loading vectors of different lengths", "[slow]")
{
    constexpr auto for_each_generated_length =
        [](auto init, int count, auto&& process) {
            process(init);
            for (int i = 0; i < count; ++i) {
                init = std::move(init).push_back(i);
                process(init);
            }
        };

    SECTION("Persist each vector by itself")
    {
        for_each_generated_length(
            test::vector_one<int>{}, 350, [&](const auto& vec) {
                auto pool           = example_output_pool{};
                auto id1            = immer::persist::container_id{};
                std::tie(pool, id1) = add_to_pool(vec, pool);

                {
                    // Loads correctly
                    auto loader        = example_loader{to_input_pool(pool)};
                    const auto loaded1 = loader.load_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }

    SECTION("keep archiving into the same pool")
    {
        auto pool = example_output_pool{};
        for_each_generated_length(
            test::vector_one<int>{}, 350, [&](const auto& vec) {
                auto id1            = immer::persist::container_id{};
                std::tie(pool, id1) = add_to_pool(vec, pool);

                {
                    // Loads correctly
                    auto loader        = example_loader{to_input_pool(pool)};
                    const auto loaded1 = loader.load_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }
}

TEST_CASE("Test flex vectors memory leak")
{
    constexpr auto for_each_generated_length_flex =
        [](auto init, int count, auto&& process) {
            for (int i = 0; i < count; ++i) {
                init = std::move(init).push_back(i);
                process(init);
            }
        };

    constexpr auto max_length = 350;

    SECTION("Loader deallocates nodes")
    {
        auto loaders = std::vector<example_loader>{};

        for_each_generated_length_flex(
            test::flex_vector_one<int>{}, max_length, [&](const auto& vec) {
                auto pool           = example_output_pool{};
                auto id1            = immer::persist::container_id{};
                std::tie(pool, id1) = add_to_pool(vec, pool);

                {
                    // Loads correctly
                    loaders.emplace_back(to_input_pool(pool));
                    auto& loader       = loaders.back();
                    const auto loaded1 = loader.load_flex_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }
    SECTION("Inner node should deallocate its children")
    {
        /**
         * The goal is to trigger the branch where the loader has to deallocate
         * an inner node AND its children which also must be inner nodes.
         *
         * It's not that easy because if a child is a leaf, leaves are stored in
         * the separate map `immer::map<node_id, node_ptr> leaves_` and
         * therefore, will never be deallocated by an inner node.
         *
         * So it must be an inner node that has the last pointer to another
         * inner node.
         */
        auto pool = example_output_pool{};
        auto ids  = std::vector<immer::persist::container_id>{};
        auto vecs = std::vector<test::flex_vector_one<int>>{};

        for_each_generated_length_flex(
            test::flex_vector_one<int>{}, max_length, [&](const auto& vec) {
                auto id1            = immer::persist::container_id{};
                std::tie(pool, id1) = add_to_pool(vec, pool);
                ids.push_back(id1);
                vecs.push_back(vec);
            });

        auto index = std::size_t{};
        for (const auto& id : ids) {
            auto loader        = example_loader{to_input_pool(pool)};
            const auto loaded1 = loader.load_flex_vector(id);
            REQUIRE(loaded1 == vecs[index]);
            ++index;
        }
    }
    SECTION("Vector deallocates nodes")
    {
        for_each_generated_length_flex(
            test::flex_vector_one<int>{}, max_length, [&](const auto& vec) {
                auto pool           = example_output_pool{};
                auto id1            = immer::persist::container_id{};
                std::tie(pool, id1) = add_to_pool(vec, pool);

                {
                    // Loads correctly
                    const auto loaded1 =
                        example_loader{to_input_pool(pool)}.load_flex_vector(
                            id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }
}

TEST_CASE("Test saving and loading flex vectors of different lengths", "[slow]")
{
    constexpr auto for_each_generated_length_flex =
        [](auto init, int count, auto&& process) {
            // Combine flex and non-flex
            process(init);
            process(flex_vector_one<int>{init});
            process(flex_vector_one<int>{init} + init);
            for (int i = 0; i < count; ++i) {
                auto prev = init;
                init      = std::move(init).push_back(i);

                process(init);
                process(flex_vector_one<int>{prev} + init);
                process(flex_vector_one<int>{init} + prev);
                process(flex_vector_one<int>{init} + init);
            }
        };

    SECTION("one vector per pool")
    {
        for_each_generated_length_flex(
            test::flex_vector_one<int>{}, 350, [&](const auto& vec) {
                auto pool           = example_output_pool{};
                auto id1            = immer::persist::container_id{};
                std::tie(pool, id1) = add_to_pool(vec, pool);

                {
                    // Loads correctly
                    auto loader        = example_loader{to_input_pool(pool)};
                    const auto loaded1 = loader.load_flex_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }

    SECTION("one pool for all")
    {
        auto pool = example_output_pool{};
        for_each_generated_length_flex(
            test::vector_one<int>{}, 350, [&](const auto& vec) {
                auto id1            = immer::persist::container_id{};
                std::tie(pool, id1) = add_to_pool(vec, pool);

                // Loads correctly
                auto loader        = make_loader_for(vec, to_input_pool(pool));
                const auto loaded1 = loader.load(id1);
                REQUIRE(loaded1 == vec);
            });
    }
}

TEST_CASE("A loop with 2 nodes")
{
    const auto json = std::string{R"(
{
    "value0": {
        "B": 5,
        "BL": 1,
        "leaves": [
            [32, [58, 59]],
            [34, [62, 63]],
            [3, [0, 1]],
            [5, [4, 5]],
            [6, [6, 7]],
            [7, [8, 9]],
            [8, [10, 11]],
            [9, [12, 13]],
            [10, [14, 15]],
            [11, [16, 17]],
            [12, [18, 19]],
            [13, [20, 21]],
            [14, [22, 23]],
            [15, [24, 25]],
            [16, [26, 27]],
            [17, [28, 29]],
            [18, [30, 31]],
            [19, [32, 33]],
            [20, [34, 35]],
            [21, [36, 37]],
            [22, [38, 39]],
            [23, [40, 41]],
            [24, [42, 43]],
            [25, [44, 45]],
            [26, [46, 47]],
            [27, [48, 49]],
            [28, [50, 51]],
            [29, [52, 53]],
            [30, [54, 55]],
            [31, [56, 57]],
            [1, [66]],
            [33, [60, 61]],
            [4, [2, 3]],
            [36, [64, 65]]
        ],
        "inners": [
            [0, {
                "children": [2, 35],
                "relaxed": false
            }],
            [35, {
                "children": [36, 0],
                "relaxed": false
            }],
            [2, {
                "children": [3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34],
                "relaxed": false
            }]
        ],
        "vectors": [
            {
                "root": 0,
                "tail": 1
            }
        ]
    }
}
)"};
    REQUIRE_THROWS_AS(load_flex_vec(json, 0), immer::persist::pool_has_cycles);
}

namespace {
struct big_object
{
    double a, b, c, d, e;

    big_object(int val)
        : a{static_cast<double>(val)}
        , b{a * 10}
        , c{a * 100}
        , d{a * 1000}
        , e{a * 10000}
    {
    }

    auto tie() const { return std::tie(a, b, c, d, e); }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(a),
           CEREAL_NVP(b),
           CEREAL_NVP(c),
           CEREAL_NVP(d),
           CEREAL_NVP(e));
    }

    friend bool operator==(const big_object& left, const big_object& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const big_object& value)
    {
        return s << fmt::format("({}, {}, {}, {}, {})",
                                value.a,
                                value.b,
                                value.c,
                                value.d,
                                value.e);
    }
};

template <class T>
class show_type;

template <class T>
auto get_node_for()
{
    using rbtree_t = std::decay_t<decltype(test::vector_one<T>{}.impl())>;
    using node_t   = typename rbtree_t::node_t;
    return boost::hana::type_c<node_t>;
}

template <class T>
using node_for = typename decltype(get_node_for<T>())::type;
} // namespace

TEST_CASE("Test vector with very big objects")
{
    // There are always two branches, two objects in the leaf.
    static_assert(immer::detail::rbts::branches<1> == 2);

    // Even when the object is big, immer::vector puts up to 2 of them in a
    // leaf.
    using big_object_node_t = node_for<big_object>;
    static_assert(big_object_node_t::bits_leaf == 1);
    static_assert(sizeof(big_object) == 40);
    // 96 is in Debug probably because 40 is aligned to 48, times two.
    // 88 is in Release. The actual number doesn't really matter, it's just here
    // to have an idea of the sizes we're dealing with.
    static_assert(big_object_node_t::max_sizeof_leaf == 96 ||
                  big_object_node_t::max_sizeof_leaf == 88);

    using int_node_t = node_for<int>;
    static_assert(int_node_t::bits_leaf == 1);
    static_assert(sizeof(int) == 4);
    // show_type<boost::hana::size_t<int_node_t::max_sizeof_leaf>> show;
    static_assert(int_node_t::max_sizeof_leaf == 24 ||
                  int_node_t::max_sizeof_leaf == 16);

    const auto small_vec = gen(test::vector_one<big_object>{}, 67);

    auto pool = immer::persist::rbts::make_output_pool_for(
        test::vector_one<big_object>{});
    auto id1            = immer::persist::container_id{};
    std::tie(pool, id1) = add_to_pool(small_vec, pool);

    {
        // Loads correctly
        auto loader = immer::persist::rbts::make_loader_for(
            test::vector_one<big_object>{}, to_input_pool(pool));
        const auto loaded1 = loader.load(id1);
        REQUIRE(loaded1 == small_vec);
    }

    // REQUIRE(to_json(pool) == "");
}

TEST_CASE("Test modifying vector nodes")
{
    json_t data;
    data["value0"]["B"]      = 5;
    data["value0"]["BL"]     = 1;
    data["value0"]["leaves"] = {
        {1, {6}},
        {2, {0, 1}},
        {3, {2, 3}},
        {4, {4, 5}},
    };
    data["value0"]["inners"] = {
        {
            0,
            {
                {"children", {2, 3, 4}},
                {"relaxed", false},
            },
        },
    };
    data["value0"]["vectors"] = {
        {
            {"root", 0},
            {"tail", 1},
        },
    };
    data["value0"]["flex_vectors"] = json_t::array();

    SECTION("Loads correctly")
    {
        const auto vec = test::vector_one<int>{0, 1, 2, 3, 4, 5, 6};
        REQUIRE(load_vec(data.dump(), 0) == vec);
    }

    SECTION("Load non-existing vector")
    {
        REQUIRE_THROWS_AS(load_vec(data.dump(), 99),
                          immer::persist::pool_exception);
    }

    SECTION("Load different B")
    {
        auto b              = GENERATE(1, 3, 6, 7);
        data["value0"]["B"] = b;
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer::persist::rbts::incompatible_bits_parameters);
    }

    SECTION("Load different BL")
    {
        auto bl              = GENERATE(2, 3, 5, 6);
        data["value0"]["BL"] = bl;
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer::persist::rbts::incompatible_bits_parameters);
    }

    SECTION("Invalid root id")
    {
        data["value0"]["vectors"][0]["root"] = 1;
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer::persist::invalid_node_id);
    }

    SECTION("Invalid tail id")
    {
        data["value0"]["vectors"][0]["tail"] = 999;
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer::persist::invalid_node_id);
    }

    SECTION("A leaf with too few elements")
    {
        auto& item = data["value0"]["leaves"][2];
        REQUIRE(item[0] == 3);
        // Leaf #3 should have two elements, but it has only one.
        item[1] = {2};
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer::persist::rbts::vector_corrupted_exception);
    }

    SECTION("Mess with the tail")
    {
        auto& item = data["value0"]["leaves"][0];
        REQUIRE(item[0] == 1);
        SECTION("Add to the tail")
        {
            item[1]        = {6, 7};
            const auto vec = test::vector_one<int>{0, 1, 2, 3, 4, 5, 6, 7};
            REQUIRE(load_vec(data.dump(), 0) == vec);
        }
        SECTION("Remove from the tail")
        {
            item[1] = json_t::array();
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer::persist::rbts::vector_corrupted_exception);
        }
        SECTION("Add too many elements")
        {
            // Three elements can't be in a leaf
            item[1] = {6, 7, 8};
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer::persist::invalid_children_count);
        }
    }

    SECTION("Modify children")
    {
        auto& item = data["value0"]["inners"][0];
        REQUIRE(item[0] == 0);
        SECTION("Add too many")
        {
            item[1]["children"] = std::vector<int>(33, 2);
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer::persist::invalid_children_count);
        }
        SECTION("Remove one")
        {
            item[1]["children"] = {2, 4};
            const auto vec      = test::vector_one<int>{0, 1, 4, 5, 6};
            REQUIRE(load_vec(data.dump(), 0) == vec);
        }
        SECTION("Unknown child")
        {
            item[1]["children"] = {2, 4, 9};
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer::persist::invalid_node_id);
        }
        SECTION("Node has itself as a child")
        {
            item[1]["children"] = {2, 0, 4};
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer::persist::pool_has_cycles);
        }
        SECTION("Strict vector can not have relaxed nodes")
        {
            item[1]["relaxed"] = true;
            REQUIRE_THROWS_AS(
                load_vec(data.dump(), 0),
                immer::persist::rbts::relaxed_node_not_allowed_exception);
        }
    }
}

TEST_CASE("Test modifying flex vector nodes")
{
    json_t data;
    data["value0"]["B"]      = 5;
    data["value0"]["BL"]     = 1;
    data["value0"]["leaves"] = {
        {1, {6, 99}},
        {2, {0, 1}},
        {3, {2, 3}},
        {4, {4, 5}},
        {5, {6}},
    };
    data["value0"]["inners"] = {
        {0, {{"children", {2, 3, 4, 5, 2, 3, 4}}, {"relaxed", true}}},
        {902, {{"children", {2, 2, 3, 4}}, {"relaxed", true}}},
    };
    data["value0"]["vectors"] = {
        {
            {"root", 0},
            {"tail", 1},
        },
    };

    SECTION("Loads correctly")
    {
        const auto small_vec = gen(test::flex_vector_one<int>{}, 7);
        const auto vec = small_vec + small_vec + test::flex_vector_one<int>{99};
        REQUIRE(load_flex_vec(data.dump(), 0) == vec);
    }
    SECTION("Load non-existing vector")
    {
        REQUIRE_THROWS_AS(load_flex_vec(data.dump(), 99),
                          immer::persist::pool_exception);
    }
    SECTION("Non-relaxed vector can not have relaxed nodes")
    {
        REQUIRE_THROWS_AS(
            load_vec(data.dump(), 0),
            immer::persist::rbts::relaxed_node_not_allowed_exception);
    }
    SECTION("Modify starting leaf")
    {
        auto& item = data["value0"]["leaves"][1];
        REQUIRE(item[0] == 2);
        SECTION("One element")
        {
            item[1]            = {0};
            const auto vec_234 = test::flex_vector_one<int>{0, 2, 3, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }

        // Empty starting leaf triggers a separate branch in the code, actually.
        SECTION("Empty leaf")
        {
            item[1]            = json_t::array();
            const auto vec_234 = test::flex_vector_one<int>{2, 3, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }
    }
    SECTION("Modify second leaf")
    {
        auto& item = data["value0"]["leaves"][2];
        REQUIRE(item[0] == 3);
        SECTION("One element")
        {
            item[1]            = {2};
            const auto vec_234 = test::flex_vector_one<int>{0, 1, 2, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }
        SECTION("Empty leaf")
        {
            item[1]            = json_t::array();
            const auto vec_234 = test::flex_vector_one<int>{0, 1, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }
    }
    SECTION("Modify inner node")
    {
        auto& item = data["value0"]["inners"][0];
        REQUIRE(item[0] == 0);
        SECTION("Mix leaves and inners as children")
        {
            auto& children = item[1]["children"];
            REQUIRE(children == json_t::array({2, 3, 4, 5, 2, 3, 4}));
            children = {2, 3, 4, 902, 4};
            REQUIRE_THROWS_AS(
                load_flex_vec(data.dump(), 0),
                immer::persist::rbts::same_depth_children_exception);
        }
        SECTION("No children")
        {
            item[1]["children"] = json_t::array();
            // There was a problem when an empty relaxed node triggered an
            // assert, while non-relaxed worked fine.
            const auto is_relaxed = GENERATE(false, true);
            item[1]["relaxed"]    = is_relaxed;
            const auto leaf_1     = test::flex_vector_one<int>{6, 99};
            REQUIRE(load_flex_vec(data.dump(), 0) == leaf_1);
        }
        SECTION("Too many children")
        {
            item[1]["children"]   = std::vector<int>(40, 3);
            const auto is_relaxed = GENERATE(false, true);
            item[1]["relaxed"]    = is_relaxed;
            REQUIRE_THROWS_AS(load_flex_vec(data.dump(), 0),
                              immer::persist::invalid_children_count);
        }
        SECTION("Remove a child")
        {
            SECTION("All leaves are full, non-relaxed works")
            {
                item[1]["children"]   = {2, 3, 4, 2, 3, 4};
                const auto is_relaxed = GENERATE(false, true);
                item[1]["relaxed"]    = is_relaxed;
                const auto leaf_1     = test::flex_vector_one<int>{6, 99};
                const auto vec_234 =
                    test::flex_vector_one<int>{0, 1, 2, 3, 4, 5};
                const auto vec = vec_234 + vec_234 + leaf_1;
                REQUIRE(load_flex_vec(data.dump(), 0) == vec);
            }
            SECTION("Relaxed leaf")
            {
                item[1]["children"] = {2, 3, 5, 3};
                SECTION("Non-relaxed breaks")
                {
                    item[1]["relaxed"] = false;
                    REQUIRE_THROWS_AS(
                        load_flex_vec(data.dump(), 0),
                        immer::persist::rbts::vector_corrupted_exception);
                }
                SECTION("Relaxed works")
                {
                    const auto leaf_1 = test::flex_vector_one<int>{6, 99};
                    const auto leaf_2 = test::flex_vector_one<int>{0, 1};
                    const auto leaf_3 = test::flex_vector_one<int>{2, 3};
                    const auto leaf_5 = test::flex_vector_one<int>{6};

                    const auto vec = leaf_2 + leaf_3 + leaf_5 + leaf_3 + leaf_1;
                    REQUIRE(load_flex_vec(data.dump(), 0) == vec);
                }
            }
        }
    }
}

TEST_CASE("Print shift calculation", "[.print_shift]")
{
    // It starts with BL, which is 1.
    // Then grows by B, which is 5 by default.
    // size [0, 66], shift 1
    // size [67, 2050], shift 6
    // size [2051, 65538], shift 11
    auto vec        = example_vector{};
    auto last_shift = vec.impl().shift;
    for (auto index = 0; index < 90000; ++index) {
        vec              = vec.push_back(index);
        const auto shift = vec.impl().shift;
        if (shift != last_shift) {
            last_shift = shift;
        }
    }
}

TEST_CASE("Test more inner nodes")
{
    json_t data;
    data["value0"]["B"]      = 5;
    data["value0"]["BL"]     = 1;
    data["value0"]["leaves"] = {
        {32, {58, 59}}, {34, {62, 63}}, {3, {0, 1}},    {5, {4, 5}},
        {6, {6, 7}},    {7, {8, 9}},    {8, {10, 11}},  {9, {12, 13}},
        {10, {14, 15}}, {11, {16, 17}}, {12, {18, 19}}, {13, {20, 21}},
        {14, {22, 23}}, {15, {24, 25}}, {16, {26, 27}}, {17, {28, 29}},
        {18, {30, 31}}, {19, {32, 33}}, {20, {34, 35}}, {21, {36, 37}},
        {22, {38, 39}}, {23, {40, 41}}, {24, {42, 43}}, {25, {44, 45}},
        {26, {46, 47}}, {27, {48, 49}}, {28, {50, 51}}, {29, {52, 53}},
        {30, {54, 55}}, {31, {56, 57}}, {1, {66}},      {33, {60, 61}},
        {4, {2, 3}},    {36, {64, 65}},
    };
    data["value0"]["inners"] = {
        {0, {{"children", {2, 35}}, {"relaxed", false}}},
        {2,
         {{"children",
           {3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
            19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34}},
          {"relaxed", false}}},
        {35, {{"children", {36}}, {"relaxed", false}}},
    };
    data["value0"]["vectors"] = {
        {
            {"root", 0},
            {"tail", 1},
        },
    };
    data["value0"]["flex_vectors"] = json_t::array();

    SECTION("Loads correctly")
    {
        REQUIRE(load_vec(data.dump(), 0) == gen(example_vector{}, 67));
    }
    SECTION("Throw in some empty nodes")
    {
        // Empty leaf #205
        data["value0"]["leaves"].push_back({205, json_t::array()});
        // Inner node #560 referencing the empty leaf
        data["value0"]["inners"].push_back(
            {560, {{"children", {205}}, {"relaxed", false}}});
        auto& item = data["value0"]["inners"][0];
        SECTION("Three nodes")
        {
            item[1]["children"] = {2, 560, 35};
            REQUIRE(load_vec(data.dump(), 0) == gen(example_vector{}, 67));
        }

        /**
         * NOTE: These two tests do not work as long as there is a bug in "Test
         * flex vector with a weird shape strict".
         */
        // SECTION("Empty first")
        // {
        //     FAIL("Fix this test");
        //     item[1]["children"] = {560, 35};
        //     REQUIRE_NOTHROW(load_vec(data.dump(), 0));
        //     REQUIRE(1 == 2);
        //     // REQUIRE(load_vec(data.dump(), 0) == example_vector{64, 65,
        //     66});
        // }
        // SECTION("Empty last")
        // {
        //     FAIL("Fix this test");
        //     item[1]["children"] = {35, 560};
        //     REQUIRE(load_vec(data.dump(), 0) == example_vector{64, 65, 66});
        // }
    }
    SECTION("Mix leaves and inners as children")
    {
        auto& children = data["value0"]["inners"][0][1]["children"];
        REQUIRE(children == json_t::array({2, 35}));
        children = {2, 28};
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer::persist::rbts::same_depth_children_exception);
    }
    SECTION("Strict vector can't contain relaxed nodes at all")
    {
        SECTION("Relaxed root, 0")
        {
            auto& inners = data["value0"]["inners"];
            auto& item   = inners[0][1];
            REQUIRE_FALSE(item["relaxed"]);
            item["relaxed"] = true;
            REQUIRE_THROWS_AS(
                load_vec(data.dump(), 0),
                immer::persist::rbts::relaxed_node_not_allowed_exception);
        }
        SECTION("Relaxed non-root")
        {
            auto& inners = data["value0"]["inners"];
            auto& item   = inners[1][1];
            REQUIRE_FALSE(item["relaxed"]);
            item["relaxed"] = true;
            REQUIRE_THROWS_AS(
                load_vec(data.dump(), 0),
                immer::persist::rbts::relaxed_node_not_allowed_exception);
        }
    }
    SECTION("Flex vector loads as well")
    {
        REQUIRE(load_flex_vec(data.dump(), 0) == gen(example_vector{}, 67));

        auto& inners = data["value0"]["inners"];
        SECTION("Relaxed 0 can contain non-relaxed")
        {
            auto& item = inners[0][1];
            REQUIRE_FALSE(item["relaxed"]);
            item["relaxed"] = true;
            REQUIRE(load_flex_vec(data.dump(), 0) == gen(example_vector{}, 67));

            SECTION("Relaxed 0 can contain relaxed 2")
            {
                inners[1][1]["relaxed"] = true;
                REQUIRE(load_flex_vec(data.dump(), 0) ==
                        gen(example_vector{}, 67));
            }
        }
        SECTION("Strict 0 can't contain relaxed 2")
        {
            inners[1][1]["relaxed"]  = true;
            inners[1][1]["children"] = {3, 4, 5};
            REQUIRE_THROWS_AS(
                load_flex_vec(data.dump(), 0),
                immer::persist::rbts::relaxed_node_not_allowed_exception);
        }
    }
}

TEST_CASE("Exception while loading children")
{
    json_t data;
    data["value0"]["B"]      = 5;
    data["value0"]["BL"]     = 1;
    data["value0"]["leaves"] = {
        {1, {6, 99}},
        {2, {0, 1}},
        {3, {2, 3}},
        {4, {4, 5}},
        {5, {6}},
    };
    data["value0"]["inners"] = {
        {0, {{"children", {2, 3, 4, 5, 2, 3, 4}}, {"relaxed", true}}},
        {902, {{"children", {5}}, {"relaxed", true}}},
    };
    data["value0"]["vectors"] = {
        {
            {"root", 0},
            {"tail", 1},
        },
    };

    auto& children = data["value0"]["inners"][0][1]["children"];
    REQUIRE(children == json_t::array({2, 3, 4, 5, 2, 3, 4}));
    children = {2, 3, 4, 902, 4};
    REQUIRE_THROWS_AS(load_flex_vec(data.dump(), 0),
                      immer::persist::rbts::same_depth_children_exception);
}

TEST_CASE("Test flex vector with a weird shape relaxed")
{
    json_t data;
    data["value0"]["B"]      = 5;
    data["value0"]["BL"]     = 1;
    data["value0"]["leaves"] = {
        {1, {66}},
        {36, {64, 65}},
    };
    data["value0"]["inners"] = {
        {0, {{"children", {35}}, {"relaxed", true}}},
        {35, {{"children", {36}}, {"relaxed", true}}},
    };
    data["value0"]["vectors"] = {
        {
            {"root", 0},
            {"tail", 1},
        },
    };

    const auto loaded = load_flex_vec(data.dump(), 0);
    // {
    //     auto pool        =
    //     immer::persist::rbts::make_output_pool_for(loaded); auto vector_id =
    //     immer::persist::node_id{}; std::tie(pool, vector_id) =
    //         immer::persist::rbts::add_to_pool(loaded, pool);
    // }

    const auto expected = example_vector{64, 65, 66};

    // Test iteration over the loaded vector
    {
        const auto rebuilt = example_vector{loaded.begin(), loaded.end()};
        REQUIRE(rebuilt == expected);
    }

    REQUIRE(loaded == expected);
}

TEST_CASE("Test flex vector with a weird shape strict", "[.broken]")
{
    json_t data;
    data["value0"]["B"]      = 5;
    data["value0"]["BL"]     = 1;
    data["value0"]["leaves"] = {
        {1, {66}},
        {36, {64, 65}},
    };
    data["value0"]["inners"] = {
        {0, {{"children", {35}}, {"relaxed", false}}},
        {35, {{"children", {36}}, {"relaxed", false}}},
    };
    data["value0"]["vectors"] = {
        {{"root", 0}, {"tail", 1}},
    };

    const auto loaded = load_flex_vec(data.dump(), 0);
    // {
    //     auto pool        =
    //     immer::persist::rbts::make_output_pool_for(loaded); auto vector_id =
    //     immer::persist::node_id{}; std::tie(pool, vector_id) =
    //         immer::persist::rbts::add_to_pool(loaded, pool);
    // }

    const auto expected = example_vector{64, 65, 66};

    // Test iteration over the loaded vector
    {
        const auto rebuilt = example_vector{loaded.begin(), loaded.end()};
        REQUIRE(rebuilt == expected);
    }

    REQUIRE(loaded == expected);
}

TEST_CASE("Flex vector converted from strict")
{
    /**
     * 1. Save a normal vector
     * 2. Convert into flex and save again. This is no-op and the ID is the same
     * as before.
     * 3. Consequently, you can load any normal vector as a flex-vector
     */
    const auto small_vec      = gen(test::vector_one<int>{}, 67);
    const auto small_flex_vec = test::flex_vector_one<int>{small_vec};

    auto pool         = immer::persist::rbts::make_output_pool_for(small_vec);
    auto small_vec_id = immer::persist::container_id{};
    auto small_flex_vec_id = immer::persist::container_id{};

    SECTION("First save strict")
    {
        std::tie(pool, small_vec_id)      = add_to_pool(small_vec, pool);
        std::tie(pool, small_flex_vec_id) = add_to_pool(small_flex_vec, pool);
    }
    SECTION("First save flex")
    {
        std::tie(pool, small_flex_vec_id) = add_to_pool(small_flex_vec, pool);
        std::tie(pool, small_vec_id)      = add_to_pool(small_vec, pool);
    }

    // The id is the same
    REQUIRE(small_flex_vec_id == small_vec_id);

    // Can be loaded either way, as flex or normal
    {
        auto loader_flex = make_loader_for(small_flex_vec, to_input_pool(pool));
        REQUIRE(small_flex_vec == loader_flex.load(small_flex_vec_id));
    }
    {
        auto loader = make_loader_for(small_vec, to_input_pool(pool));
        REQUIRE(small_vec == loader.load(small_vec_id));
    }
}

TEST_CASE("Can't load saved flex vector with relaxed nodes as strict")
{
    const auto small_vec = gen(test::flex_vector_one<int>{}, 67);
    const auto vec       = small_vec + small_vec;
    auto pool            = immer::persist::rbts::make_output_pool_for(vec);
    auto vec_id          = immer::persist::container_id{};

    std::tie(pool, vec_id) = add_to_pool(vec, pool);
    SECTION("Flex loads well")
    {
        auto loader_flex =
            make_loader_for(test::flex_vector_one<int>{}, to_input_pool(pool));
        REQUIRE(vec == loader_flex.load(vec_id));
    }
    SECTION("Strict can't load")
    {
        auto loader =
            make_loader_for(test::vector_one<int>{}, to_input_pool(pool));
        REQUIRE_THROWS_AS(
            loader.load(vec_id),
            immer::persist::rbts::relaxed_node_not_allowed_exception);
    }
}

TEST_CASE("Vector: converting loader can handle exceptions")
{
    const auto vec                = example_vector{1, 2, 3};
    const auto [out_pool, vec_id] = add_to_pool(vec, {});
    const auto in_pool            = to_input_pool(out_pool);

    using Pool = std::decay_t<decltype(in_pool)>;

    SECTION("Transformation works")
    {
        constexpr auto transform = [](int val) {
            return fmt::format("_{}_", val);
        };
        const auto transform_vector = [transform](const auto& vec) {
            auto result = test::vector_one<std::string>{};
            for (const auto& item : vec) {
                result = std::move(result).push_back(transform(item));
            }
            return result;
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader =
            immer::persist::rbts::loader<std::string,
                                         immer::default_memory_policy,
                                         immer::default_bits,
                                         1,
                                         Pool,
                                         TransformF>;
        auto loader       = Loader{in_pool, transform};
        const auto loaded = loader.load_vector(vec_id);
        REQUIRE(loaded == transform_vector(vec));
    }

    SECTION("Exception is handled")
    {
        constexpr auto transform = [](int val) {
            if (val == 2) {
                throw std::runtime_error{"it's two much"};
            }
            return fmt::format("_{}_", val);
        };

        using TransformF = std::decay_t<decltype(transform)>;
        using Loader =
            immer::persist::rbts::loader<std::string,
                                         immer::default_memory_policy,
                                         immer::default_bits,
                                         1,
                                         Pool,
                                         TransformF>;
        auto loader = Loader{in_pool, transform};
        REQUIRE_THROWS_WITH(loader.load_vector(vec_id), "it's two much");
    }
}
