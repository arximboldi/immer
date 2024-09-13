#include <catch2/catch_test_macros.hpp>

#include <immer/extra/persist/cereal/save.hpp>
#include <immer/extra/persist/transform.hpp>

#include "utils.hpp"

#include <immer/extra/cereal/immer_box.hpp>
#include <immer/extra/cereal/immer_set.hpp>
#include <immer/extra/cereal/immer_table.hpp>

#include <cereal/archives/xml.hpp>
#include <nlohmann/json.hpp>

#define DEFINE_OPERATIONS(name)                                                \
    bool operator==(const name& left, const name& right)                       \
    {                                                                          \
        return members(left) == members(right);                                \
    }                                                                          \
    bool operator!=(const name& left, const name& right)                       \
    {                                                                          \
        return members(left) != members(right);                                \
    }                                                                          \
    template <class Archive>                                                   \
    void serialize(Archive& ar, name& m)                                       \
    {                                                                          \
        serialize_members(ar, m);                                              \
    }

namespace {

using test::flex_vector_one;
using test::members;
using test::serialize_members;
using test::vector_one;
namespace hana = boost::hana;
using json_t   = nlohmann::json;

template <class T>
class show_type;

} // namespace

namespace model {

struct value_two;

struct two_boxed
{
    BOOST_HANA_DEFINE_STRUCT(two_boxed, (immer::box<value_two>, two));

    two_boxed() = default;
    explicit two_boxed(value_two val);
};

struct key
{
    BOOST_HANA_DEFINE_STRUCT(key, (std::string, str));

    friend std::size_t xx_hash_value(const key& value)
    {
        return immer::persist::xx_hash_value_string(value.str);
    }
};

const key& get_table_key(const two_boxed& two);

struct value_one
{
    BOOST_HANA_DEFINE_STRUCT(
        value_one, //
                   //  (immer::box<std::string>, box_test)
        (vector_one<two_boxed>, twos),
        (flex_vector_one<two_boxed>, twos_flex),
        (immer::table<two_boxed,
                      immer::table_key_fn,
                      immer::persist::xx_hash<key>>,
         twos_table),
        (immer::table<two_boxed,
                      immer::table_key_fn,
                      immer::persist::xx_hash<key>>,
         twos_table_2),
        (immer::map<key, two_boxed, immer::persist::xx_hash<key>>, twos_map),
        (immer::set<two_boxed, immer::persist::xx_hash<two_boxed>>, twos_set)

    );
};

struct value_two
{
    int number                 = {};
    vector_one<value_one> ones = {};
    key key                    = {};

    friend std::ostream& operator<<(std::ostream& s, const value_two& value)
    {
        return s << fmt::format("number = {}, ones = {}, key = '{}'",
                                value.number,
                                value.ones.size(),
                                value.key.str);
    }
};

std::ostream& operator<<(std::ostream& s, const two_boxed& value)
{
    return s << value.two.get();
}

const key& get_table_key(const two_boxed& two) { return two.two.get().key; }

std::size_t xx_hash_value(const two_boxed& value)
{
    return xx_hash_value(value.two.get().key);
}

two_boxed::two_boxed(value_two val)
    : two{val}
{
}

} // namespace model

template <>
struct fmt::formatter<model::two_boxed> : ostream_formatter
{};

BOOST_HANA_ADAPT_STRUCT(model::value_two, number, ones, key);

namespace model {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(key);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);
} // namespace model

namespace format {

struct value_two;

struct two_boxed
{
    BOOST_HANA_DEFINE_STRUCT(two_boxed, (immer::box<value_two>, two));

    two_boxed() = default;
    explicit two_boxed(immer::box<value_two> two_);
};

struct key
{
    BOOST_HANA_DEFINE_STRUCT(key, (std::string, str));

    friend std::size_t xx_hash_value(const key& value)
    {
        return immer::persist::xx_hash_value_string(value.str);
    }
};

const key& get_table_key(const two_boxed& two);

struct value_one
{
    BOOST_HANA_DEFINE_STRUCT(
        value_one, //
                   //  (immer::box<std::string>, box_test)
        (vector_one<two_boxed>, twos),
        (flex_vector_one<two_boxed>, twos_flex),
        (immer::table<two_boxed,
                      immer::table_key_fn,
                      immer::persist::xx_hash<key>>,
         twos_table),
        (immer::table<two_boxed,
                      immer::table_key_fn,
                      immer::persist::xx_hash<key>>,
         twos_table_2),
        (immer::map<key, two_boxed, immer::persist::xx_hash<key>>, twos_map),
        (immer::set<two_boxed, immer::persist::xx_hash<two_boxed>>, twos_set)

    );
};

struct value_two
{
    int number                 = {};
    vector_one<value_one> ones = {};
    key key                    = {};
};

const key& get_table_key(const two_boxed& two) { return two.two.get().key; }

std::size_t xx_hash_value(const two_boxed& value)
{
    return xx_hash_value(value.two.get().key);
}

two_boxed::two_boxed(immer::box<value_two> two_)
    : two{std::move(two_)}
{
}

} // namespace format

BOOST_HANA_ADAPT_STRUCT(format::value_two, number, ones, key);

namespace format {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(key);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);
} // namespace format

TEST_CASE("Test exception while circular converting")
{
    const auto two1  = model::two_boxed{model::value_two{
         .number = 456,
         .key    = model::key{"456"},
    }};
    const auto two2  = model::two_boxed{model::value_two{
         .number = 123,
         .ones =
             {
                model::value_one{
                     .twos       = {two1},
                     .twos_flex  = {two1, two1},
                     .twos_table = {two1},
                     .twos_map   = {{model::key{"x_one"}, two1}},
                     .twos_set   = {two1},
                },
            },
         .key = model::key{"123"},
    }};
    const auto value = [&] {
        const auto t1 = immer::table<model::two_boxed,
                                     immer::table_key_fn,
                                     immer::persist::xx_hash<model::key>>{two1};
        const auto t2 = t1.insert(two2);
        return model::value_one{
            .twos         = {two1, two2},
            .twos_flex    = {two2, two1, two2},
            .twos_table   = t1,
            .twos_table_2 = t2,
            .twos_map = {{model::key{"one"}, two1}, {model::key{"two"}, two2}},
            .twos_set = {two2, two1},
        };
    }();

    const auto names = immer::persist::detail::get_named_pools_for_hana_type<
        model::value_one>();
    const auto model_pool = immer::persist::get_output_pools(value);

    SECTION("Try to load")
    {
        const auto json_str = test::to_json_with_auto_pool(value, names);
        const auto loaded =
            test::from_json_with_auto_pool<model::value_one>(json_str, names);
        REQUIRE(loaded == value);

        SECTION("Loaded value has the same structure as the original")
        {
            const auto json_from_loaded =
                test::to_json_with_auto_pool(loaded, names);
            REQUIRE(json_str == json_from_loaded);
        }
    }

    /**
     * NOTE: There is a circular dependency between pools: to convert
     * value_one we need to convert value_two and vice versa.
     */
    const auto convert_two_boxed = [](model::two_boxed old,
                                      const auto& convert_container) {
        const auto new_box = convert_container(
            hana::type_c<immer::box<format::value_two>>, old.two);
        return format::two_boxed{new_box};
    };
    const auto map = hana::make_map(
        hana::make_pair(hana::type_c<vector_one<model::two_boxed>>,
                        convert_two_boxed),
        hana::make_pair(hana::type_c<flex_vector_one<model::two_boxed>>,
                        convert_two_boxed),
        hana::make_pair(
            hana::type_c<immer::table<model::two_boxed,
                                      immer::table_key_fn,
                                      immer::persist::xx_hash<model::key>>>,
            hana::overload(
                [](immer::persist::target_container_type_request) {
                    return immer::table<format::two_boxed,
                                        immer::table_key_fn,
                                        immer::persist::xx_hash<format::key>>{};
                },
                convert_two_boxed)),
        hana::make_pair(
            hana::type_c<immer::set<model::two_boxed,
                                    immer::persist::xx_hash<model::two_boxed>>>,
            hana::overload(
                [](immer::persist::target_container_type_request) {
                    return immer::set<
                        format::two_boxed,
                        immer::persist::xx_hash<format::two_boxed>>{};
                },
                convert_two_boxed)),
        hana::make_pair(
            hana::type_c<immer::map<model::key,
                                    model::two_boxed,
                                    immer::persist::xx_hash<model::key>>>,
            hana::overload(
                [convert_two_boxed](std::pair<model::key, model::two_boxed> p,
                                    const auto& convert_container) {
                    return std::make_pair(
                        format::key{p.first.str},
                        convert_two_boxed(p.second, convert_container));
                },
                [](immer::persist::target_container_type_request) {
                    return immer::map<format::key,
                                      format::two_boxed,
                                      immer::persist::xx_hash<format::key>>{};
                })),
        hana::make_pair(
            hana::type_c<immer::box<model::value_two>>,
            [](model::value_two old, const auto& convert_container) {
                auto ones = convert_container(
                    hana::type_c<vector_one<format::value_one>>, old.ones);
                /**
                 * NOTE: We make a deliberate corruption while converting
                 * value_two: breaking the key. value_two is used as a key in
                 * some sets and tables, so the conversion does not preserve the
                 * structure.
                 */
                return format::value_two{
                    .number = old.number,
                    .ones   = ones,
                    .key    = format::key{"qwe"},
                };
            }),
        hana::make_pair(
            hana::type_c<vector_one<model::value_one>>,
            [](model::value_one old, const auto& convert_container) {
                auto twos = convert_container(
                    hana::type_c<vector_one<format::two_boxed>>, old.twos);
                return format::value_one{
                    .twos      = twos,
                    .twos_flex = convert_container(
                        hana::type_c<flex_vector_one<format::two_boxed>>,
                        old.twos_flex),
                    .twos_table = convert_container(
                        hana::type_c<
                            immer::table<format::two_boxed,
                                         immer::table_key_fn,
                                         immer::persist::xx_hash<format::key>>>,
                        old.twos_table),
                    .twos_table_2 = convert_container(
                        hana::type_c<
                            immer::table<format::two_boxed,
                                         immer::table_key_fn,
                                         immer::persist::xx_hash<format::key>>>,
                        old.twos_table_2),
                    .twos_map = convert_container(
                        hana::type_c<
                            immer::map<format::key,
                                       format::two_boxed,
                                       immer::persist::xx_hash<format::key>>>,
                        old.twos_map),
                    .twos_set = convert_container(
                        hana::type_c<immer::set<
                            format::two_boxed,
                            immer::persist::xx_hash<format::two_boxed>>>,
                        old.twos_set),
                };
            })

    );
    auto format_load_pools =
        immer::persist::transform_output_pool(model_pool, map);

    REQUIRE_THROWS_AS(immer::persist::convert_container(
                          model_pool, format_load_pools, value.twos),
                      immer::persist::champ::hash_validation_failed_exception);
}

TEST_CASE("Test circular dependency pools", "[conversion]")
{
    const auto two1  = model::two_boxed{model::value_two{
         .number = 456,
         .key    = model::key{"456"},
    }};
    const auto two2  = model::two_boxed{model::value_two{
         .number = 123,
         .ones =
             {
                model::value_one{
                     .twos       = {two1},
                     .twos_flex  = {two1, two1},
                     .twos_table = {two1},
                     .twos_map   = {{model::key{"x_one"}, two1}},
                     .twos_set   = {two1},
                },
            },
         .key = model::key{"123"},
    }};
    const auto value = [&] {
        const auto t1 = immer::table<model::two_boxed,
                                     immer::table_key_fn,
                                     immer::persist::xx_hash<model::key>>{two1};
        const auto t2 = t1.insert(two2);
        return model::value_one{
            .twos         = {two1, two2},
            .twos_flex    = {two2, two1, two2},
            .twos_table   = t1,
            .twos_table_2 = t2,
            .twos_map = {{model::key{"one"}, two1}, {model::key{"two"}, two2}},
            .twos_set = {two2, two1},
        };
    }();

    const auto names = immer::persist::detail::get_named_pools_for_hana_type<
        model::value_one>();
    const auto model_pools = immer::persist::get_output_pools(value);

    /**
     * NOTE: There is a circular dependency between pools: to convert
     * value_one we need to convert value_two and vice versa.
     */
    const auto convert_two_boxed = [](model::two_boxed old,
                                      const auto& convert_container) {
        const auto new_box = convert_container(
            hana::type_c<immer::box<format::value_two>>, old.two);
        return format::two_boxed{new_box};
    };
    const auto map = hana::make_map(
        hana::make_pair(hana::type_c<vector_one<model::two_boxed>>,
                        convert_two_boxed),
        hana::make_pair(hana::type_c<flex_vector_one<model::two_boxed>>,
                        convert_two_boxed),
        hana::make_pair(
            hana::type_c<immer::table<model::two_boxed,
                                      immer::table_key_fn,
                                      immer::persist::xx_hash<model::key>>>,
            hana::overload(
                [](immer::persist::target_container_type_request) {
                    return immer::table<format::two_boxed,
                                        immer::table_key_fn,
                                        immer::persist::xx_hash<format::key>>{};
                },
                convert_two_boxed)),
        hana::make_pair(
            hana::type_c<immer::set<model::two_boxed,
                                    immer::persist::xx_hash<model::two_boxed>>>,
            hana::overload(
                [](immer::persist::target_container_type_request) {
                    return immer::set<
                        format::two_boxed,
                        immer::persist::xx_hash<format::two_boxed>>{};
                },
                convert_two_boxed)),
        hana::make_pair(
            hana::type_c<immer::map<model::key,
                                    model::two_boxed,
                                    immer::persist::xx_hash<model::key>>>,
            hana::overload(
                [convert_two_boxed](std::pair<model::key, model::two_boxed> p,
                                    const auto& convert_container) {
                    return std::make_pair(
                        format::key{p.first.str},
                        convert_two_boxed(p.second, convert_container));
                },
                [](immer::persist::target_container_type_request) {
                    return immer::map<format::key,
                                      format::two_boxed,
                                      immer::persist::xx_hash<format::key>>{};
                })),
        hana::make_pair(
            hana::type_c<immer::box<model::value_two>>,
            [](model::value_two old, const auto& convert_container) {
                auto ones = convert_container(
                    hana::type_c<vector_one<format::value_one>>, old.ones);
                return format::value_two{
                    .number = old.number,
                    .ones   = ones,
                    .key    = format::key{old.key.str},
                };
            }),
        hana::make_pair(
            hana::type_c<vector_one<model::value_one>>,
            [](model::value_one old, const auto& convert_container) {
                auto twos = convert_container(
                    hana::type_c<vector_one<format::two_boxed>>, old.twos);
                return format::value_one{
                    .twos      = twos,
                    .twos_flex = convert_container(
                        hana::type_c<flex_vector_one<format::two_boxed>>,
                        old.twos_flex),
                    .twos_table = convert_container(
                        hana::type_c<
                            immer::table<format::two_boxed,
                                         immer::table_key_fn,
                                         immer::persist::xx_hash<format::key>>>,
                        old.twos_table),
                    .twos_table_2 = convert_container(
                        hana::type_c<
                            immer::table<format::two_boxed,
                                         immer::table_key_fn,
                                         immer::persist::xx_hash<format::key>>>,
                        old.twos_table_2),
                    .twos_map = convert_container(
                        hana::type_c<
                            immer::map<format::key,
                                       format::two_boxed,
                                       immer::persist::xx_hash<format::key>>>,
                        old.twos_map),
                    .twos_set = convert_container(
                        hana::type_c<immer::set<
                            format::two_boxed,
                            immer::persist::xx_hash<format::two_boxed>>>,
                        old.twos_set),
                };
            })

    );
    auto format_load_pools =
        immer::persist::transform_output_pool(model_pools, map);
    (void) format_load_pools;
    // show_type<decltype(format_load_pools)> qwe;

    const auto format_names =
        immer::persist::detail::get_named_pools_for_hana_type<
            format::value_one>();

    SECTION("vector")
    {
        const auto format_twos = immer::persist::convert_container(
            model_pools, format_load_pools, value.twos);

        SECTION("Same thing twice, same result")
        {
            const auto format_twos_2 = immer::persist::convert_container(
                model_pools, format_load_pools, value.twos);
            REQUIRE(format_twos.identity() == format_twos_2.identity());
        }

        // Confirm there is internal sharing happening
        REQUIRE(value.twos[0].two ==
                value.twos[1].two.get().ones[0].twos[0].two);
        REQUIRE(value.twos[0].two.impl() ==
                value.twos[1].two.get().ones[0].twos[0].two.impl());

        REQUIRE(value.twos[0].two.get().number ==
                format_twos[0].two.get().number);

        REQUIRE(format_twos[0].two ==
                format_twos[1].two.get().ones[0].twos[0].two);
        REQUIRE(format_twos[0].two.impl() ==
                format_twos[1].two.get().ones[0].twos[0].two.impl());

        REQUIRE(test::to_json(value.twos) == test::to_json(format_twos));

        SECTION("Compare structure")
        {
            const auto format_twos_json =
                test::to_json_with_auto_pool(format_twos, format_names);
            const auto model_twos_json =
                test::to_json_with_auto_pool(value.twos, names);
            REQUIRE(model_twos_json == format_twos_json);
        }
    }

    SECTION("flex_vector")
    {
        const auto format_twos = immer::persist::convert_container(
            model_pools, format_load_pools, value.twos_flex);

        SECTION("Same thing twice, same result")
        {
            const auto format_twos_2 = immer::persist::convert_container(
                model_pools, format_load_pools, value.twos_flex);
            REQUIRE(format_twos.identity() == format_twos_2.identity());
        }
        REQUIRE(test::to_json(value.twos_flex) == test::to_json(format_twos));
    }

    SECTION("table")
    {
        const auto format_twos = immer::persist::convert_container(
            model_pools, format_load_pools, value.twos_table);

        SECTION("Same thing twice, same result")
        {
            const auto format_twos_2 = immer::persist::convert_container(
                model_pools, format_load_pools, value.twos_table);
            REQUIRE(format_twos.impl().root == format_twos_2.impl().root);
        }
        REQUIRE(test::to_json(value.twos_table) == test::to_json(format_twos));
    }

    SECTION("map")
    {
        const auto format_twos = immer::persist::convert_container(
            model_pools, format_load_pools, value.twos_map);

        SECTION("Same thing twice, same result")
        {
            const auto format_twos_2 = immer::persist::convert_container(
                model_pools, format_load_pools, value.twos_map);
            REQUIRE(format_twos.impl().root == format_twos_2.impl().root);
        }

        // Confirm there is internal sharing happening
        const auto model_two1_x = value.twos_map[model::key{"one"}].two;
        const auto model_two1_y = value.twos_map[model::key{"two"}]
                                      .two.get()
                                      .ones[0]
                                      .twos_map[model::key{"x_one"}]
                                      .two;
        REQUIRE(model_two1_x == model_two1_y);
        REQUIRE(model_two1_x.impl() == model_two1_y.impl());

        const auto format_two1_x = format_twos[format::key{"one"}].two;
        const auto format_two1_y = format_twos[format::key{"two"}]
                                       .two.get()
                                       .ones[0]
                                       .twos_map[format::key{"x_one"}]
                                       .two;
        REQUIRE(format_two1_x.get().number == model_two1_x.get().number);
        REQUIRE(format_two1_x == format_two1_y);
        REQUIRE(format_two1_x.impl() == format_two1_y.impl());

        REQUIRE(test::to_json(value.twos_map) == test::to_json(format_twos));

        SECTION("Compare structure")
        {
            const auto format_twos_json =
                immer::persist::cereal_save_with_pools(
                    format_twos,
                    immer::persist::hana_struct_auto_member_name_policy(
                        format_twos));
            const auto model_twos_json = immer::persist::cereal_save_with_pools(
                value.twos_map,
                immer::persist::hana_struct_auto_member_name_policy(
                    value.twos_map));
            REQUIRE(model_twos_json == format_twos_json);
        }
    }

    SECTION("set")
    {
        const auto format_twos = immer::persist::convert_container(
            model_pools, format_load_pools, value.twos_set);

        SECTION("Same thing twice, same result")
        {
            const auto format_twos_2 = immer::persist::convert_container(
                model_pools, format_load_pools, value.twos_set);
            REQUIRE(format_twos.impl().root == format_twos_2.impl().root);
        }

        SECTION("Compare structure")
        {
            const auto format_twos_json =
                immer::persist::cereal_save_with_pools(
                    format_twos,
                    immer::persist::hana_struct_auto_member_name_policy(
                        format_twos));
            const auto model_twos_json =
                test::to_json_with_auto_pool(value.twos_set, names);
            REQUIRE(json_t::parse(model_twos_json) ==
                    json_t::parse(format_twos_json));
        }
    }

    SECTION("everything")
    {
        const auto convert = [&](const auto& value) {
            return immer::persist::convert_container(
                model_pools, format_load_pools, value);
        };
        const auto format_value = [&] {
            auto result = format::value_one{};
            hana::for_each(hana::keys(result), [&](auto key) {
                hana::at_key(result, key) = convert(hana::at_key(value, key));
            });
            return result;
        }();
        const auto format_json_str = immer::persist::cereal_save_with_pools(
            format_value,
            immer::persist::hana_struct_auto_member_name_policy(format_value));
        const auto json_str = test::to_json_with_auto_pool(value, names);
        REQUIRE(format_json_str == json_str);
    }

    SECTION("Test converting inside the pool vs outside")
    {
        const auto convert = [&](const auto& value) {
            return immer::persist::convert_container(
                model_pools, format_load_pools, value);
        };

        // Here we convert a box directly, aka "outside" the pool
        const auto two1_manually_converted = convert(two1.two);

        // While here we convert a vector which contains the box inside,
        // therefore, the box is converted internally, "inside" the pool.
        const auto two1_from_inside = convert(value.twos)[0].two;

        // The box is actually the same
        REQUIRE(two1_manually_converted.impl() == two1_from_inside.impl());
    }

    SECTION("XML also works")
    {
        const auto xml_str =
            immer::persist::cereal_save_with_pools<cereal::XMLOutputArchive>(
                value,
                immer::persist::hana_struct_auto_member_name_policy(value));
        const auto loaded_value =
            immer::persist::cereal_load_with_pools<model::value_one,
                                                   cereal::XMLInputArchive>(
                xml_str,
                immer::persist::hana_struct_auto_member_name_policy(
                    model::value_one{}));
        REQUIRE(value == loaded_value);
    }
}
