#include <catch2/catch_test_macros.hpp>

#include <immer/extra/archive/json/json_with_archive_auto.hpp>

#include "utils.hpp"

#include <immer/extra/archive/cereal/immer_box.hpp>

#define DEFINE_OPERATIONS(name)                                                \
    bool operator==(const name& left, const name& right)                       \
    {                                                                          \
        return members(left) == members(right);                                \
    }                                                                          \
    template <class Archive>                                                   \
    void serialize(Archive& ar, name& m)                                       \
    {                                                                          \
        serialize_members(ar, m);                                              \
    }

namespace {

using test::members;
using test::serialize_members;
using test::vector_one;
namespace hana = boost::hana;

} // namespace

namespace model {

struct value_two;

struct two_boxed
{
    BOOST_HANA_DEFINE_STRUCT(two_boxed, (immer::box<value_two>, two));

    two_boxed()                 = default;
    two_boxed(const two_boxed&) = default;
    explicit two_boxed(value_two val);
};

struct value_one
{
    BOOST_HANA_DEFINE_STRUCT(value_one, //
                                        //  (immer::box<std::string>, box_test)
                             (vector_one<two_boxed>, twos)

    );
};

struct value_two
{
    int number = {};
    vector_one<value_one> ones;
};

two_boxed::two_boxed(value_two val)
    : two{val}
{
}

} // namespace model

BOOST_HANA_ADAPT_STRUCT(model::value_two, number, ones);

namespace model {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);
} // namespace model

namespace format {

struct value_two;

struct two_boxed
{
    BOOST_HANA_DEFINE_STRUCT(two_boxed, (immer::box<value_two>, two));

    two_boxed()                 = default;
    two_boxed(const two_boxed&) = default;
    explicit two_boxed(immer::box<value_two> two_);
};

struct value_one
{
    BOOST_HANA_DEFINE_STRUCT(value_one, //
                                        //  (immer::box<std::string>, box_test)
                             (vector_one<two_boxed>, twos)

    );
};

struct value_two
{
    int number = {};
    vector_one<value_one> ones;
};

two_boxed::two_boxed(immer::box<value_two> two_)
    : two{std::move(two_)}
{
}

} // namespace format

BOOST_HANA_ADAPT_STRUCT(format::value_two, number, ones);

namespace format {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);
} // namespace format

TEST_CASE("Test circular dependency archives", "[conversion]")
{
    const auto two1  = model::two_boxed{model::value_two{
         .number = 456,
    }};
    const auto two2  = model::two_boxed{model::value_two{
         .number = 123,
         .ones =
             {
                model::value_one{
                     .twos = {two1},
                },
            },
    }};
    const auto value = model::value_one{
        .twos = {two1, two2},
    };

    const auto names = immer::archive::get_archives_for_types(
        hana::tuple_t<model::value_one, model::value_two, model::two_boxed>,
        hana::make_map());
    const auto [json_str, model_archives] =
        immer::archive::to_json_with_auto_archive(value, names);
    // REQUIRE(json_str == "");

    /**
     * NOTE: There is a circular dependency between archives: to convert
     * value_one we need to convert value_two and vice versa.
     */
    const auto map = hana::make_map(
        hana::make_pair(
            hana::type_c<vector_one<model::two_boxed>>,
            [](model::two_boxed old, const auto& convert_container) {
                SPDLOG_INFO("converting model::two_boxed");
                const auto new_box = convert_container(
                    hana::type_c<immer::box<format::value_two>>, old.two);
                return format::two_boxed{new_box};
            }),
        hana::make_pair(
            hana::type_c<immer::box<model::value_two>>,
            [](model::value_two old, const auto& convert_container) {
                SPDLOG_INFO("converting model::value_two");
                auto ones = convert_container(
                    hana::type_c<vector_one<format::value_one>>, old.ones);
                return format::value_two{
                    .number = old.number,
                    .ones   = ones,
                };
            }),
        hana::make_pair(
            hana::type_c<vector_one<model::value_one>>,
            [](model::value_one old, const auto& convert_container) {
                SPDLOG_INFO("converting model::value_one");
                auto twos = convert_container(
                    hana::type_c<vector_one<format::two_boxed>>, old.twos);
                return format::value_one{twos};
            })

    );
    auto format_load_archives =
        immer::archive::transform_save_archive(model_archives, map);
    (void) format_load_archives;

    const auto format_twos = immer::archive::convert_container(
        model_archives, format_load_archives, value.twos);

    SECTION("Same thing twice, same result")
    {
        const auto format_twos_2 = immer::archive::convert_container(
            model_archives, format_load_archives, value.twos);
        REQUIRE(format_twos.identity() == format_twos_2.identity());
    }

    // Confirm there is internal sharing happening
    REQUIRE(value.twos[0].two == value.twos[1].two.get().ones[0].twos[0].two);
    REQUIRE(value.twos[0].two.impl() ==
            value.twos[1].two.get().ones[0].twos[0].two.impl());

    REQUIRE(value.twos[0].two.get().number == format_twos[0].two.get().number);

    REQUIRE(format_twos[0].two == format_twos[1].two.get().ones[0].twos[0].two);
    REQUIRE(format_twos[0].two.impl() ==
            format_twos[1].two.get().ones[0].twos[0].two.impl());

    {
        const auto format_names = immer::archive::get_archives_for_types(
            hana::tuple_t<format::value_one,
                          format::value_two,
                          format::two_boxed>,
            hana::make_map());
        const auto [loaded_json_str, model_archives] =
            immer::archive::to_json_with_auto_archive(format_twos,
                                                      format_names);
        REQUIRE(test::to_json(value.twos) == test::to_json(format_twos));
    }
}
