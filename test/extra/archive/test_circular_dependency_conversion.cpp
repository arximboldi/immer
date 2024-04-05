#include <catch2/catch_test_macros.hpp>

#include <immer/extra/archive/json/json_with_archive_auto.hpp>

#include "utils.hpp"

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

    two_boxed() = default;
    two_boxed(value_two val);
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
    vector_one<value_one> ones;
};

two_boxed::two_boxed(value_two val)
    : two{val}
{
}

} // namespace model

BOOST_HANA_ADAPT_STRUCT(model::value_two, ones);

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

    two_boxed() = default;
    two_boxed(value_two val);
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
    vector_one<value_one> ones;
};

two_boxed::two_boxed(value_two val)
    : two{val}
{
}

} // namespace format

BOOST_HANA_ADAPT_STRUCT(format::value_two, ones);

namespace format {
DEFINE_OPERATIONS(two_boxed);
DEFINE_OPERATIONS(value_one);
DEFINE_OPERATIONS(value_two);
} // namespace format

TEST_CASE("Test circular dependency archives", "[.broken]")
{
    const auto two1  = model::two_boxed{};
    const auto two2  = model::two_boxed{model::value_two{
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
        hana::make_map(

            ));
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
                return format::two_boxed{convert_container(
                    hana::type_c<immer::box<format::value_two>>, old.two)};
            }),
        hana::make_pair(
            hana::type_c<immer::box<model::value_two>>,
            [](model::value_two old, const auto& convert_container) {
                SPDLOG_INFO("converting model::value_two");
                auto ones = convert_container(
                    hana::type_c<vector_one<format::value_one>>, old.ones);
                return format::value_two{ones};
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
    const auto format_load_archives =
        immer::archive::transform_save_archive(model_archives, map);
    (void) format_load_archives;
}
