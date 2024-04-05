#include <catch2/catch_test_macros.hpp>

#include <immer/extra/archive/json/json_with_archive_auto.hpp>

#include "utils.hpp"

#define DEFINE_OPERATIONS(name)                                                \
    friend bool operator==(const name& left, const name& right)                \
    {                                                                          \
        return members(left) == members(right);                                \
    }                                                                          \
    template <class Archive>                                                   \
    void serialize(Archive& ar)                                                \
    {                                                                          \
        serialize_members(ar, *this);                                          \
    }

namespace {

using test::members;
using test::serialize_members;
using test::vector_one;
namespace hana = boost::hana;

/**
 * Goal: convert between two independent hierarchies of types: model and format.
 *
 * Problem: We have a vector_one<snapshot>, which makes an archive with leaves
 * of snapshot. Normally, we need to convert model::snapshot <->
 * format::snapshot. But that involves converting between vector_one<track_id>
 * inside, which we can't do directly, otherwise we loose all structural
 * sharing, it must be done somehow via the archive, as well.
 */

namespace model {

struct track_id
{
    BOOST_HANA_DEFINE_STRUCT(track_id, //
                             (int, tid));
    DEFINE_OPERATIONS(track_id);

    track_id(int v)
        : tid{v}
    {
    }

    track_id() = default;
};

struct arrangement
{
    BOOST_HANA_DEFINE_STRUCT(arrangement, //
                             (vector_one<track_id>, tracks));

    DEFINE_OPERATIONS(arrangement);
};

struct snapshot
{
    BOOST_HANA_DEFINE_STRUCT(snapshot, //
                             (arrangement, arr));

    DEFINE_OPERATIONS(snapshot);
};

struct history
{
    BOOST_HANA_DEFINE_STRUCT(history, //
                             (vector_one<snapshot>, snapshots));

    DEFINE_OPERATIONS(history);
};

inline auto make_example_history()
{
    const auto tracks1 = vector_one<track_id>{1, 2, 3, 4, 5};
    const auto tracks2 = tracks1.push_back(6).push_back(7).push_back(8);

    return history{
        .snapshots =
            {
                snapshot{
                    .arr =
                        arrangement{
                            .tracks = tracks1,
                        },
                },
                snapshot{
                    .arr =
                        arrangement{
                            .tracks = tracks2,
                        },
                },
            },
    };
}

} // namespace model

namespace format {

struct track_id
{
    BOOST_HANA_DEFINE_STRUCT(track_id, //
                             (int, tid));
    DEFINE_OPERATIONS(track_id);
};

struct arrangement
{
    BOOST_HANA_DEFINE_STRUCT(arrangement, //
                             (vector_one<track_id>, tracks));

    DEFINE_OPERATIONS(arrangement);
};

struct snapshot
{
    BOOST_HANA_DEFINE_STRUCT(snapshot, //
                             (arrangement, arr));

    DEFINE_OPERATIONS(snapshot);
};

struct history
{
    BOOST_HANA_DEFINE_STRUCT(history, //
                             (vector_one<snapshot>, snapshots));

    DEFINE_OPERATIONS(history);
};

} // namespace format

template <class T>
class Z;

} // namespace

TEST_CASE("Convert between two hierarchies via JSON compatibility")
{
    const auto model_names = immer::archive::get_archives_for_types(
        hana::tuple_t<model::history, model::arrangement>, hana::make_map());
    const auto format_names = immer::archive::get_archives_for_types(
        hana::tuple_t<format::history, format::arrangement>, hana::make_map());
    (void) format_names;

    const auto [json_str, model_archives] =
        immer::archive::to_json_with_auto_archive(model::make_example_history(),
                                                  model_names);

    const auto map = hana::make_map(
        hana::make_pair(hana::type_c<vector_one<model::snapshot>>,
                        [](model::snapshot old, const auto& convert_container) {
                            SPDLOG_INFO("converting model::snapshot");
                            const auto format_tracks = convert_container(
                                hana::type_c<vector_one<format::track_id>>,
                                old.arr.tracks);
                            for (auto x : format_tracks) {
                                SPDLOG_INFO("x = {}", x.tid);
                            }
                            return format::snapshot{};
                        }),
        hana::make_pair(hana::type_c<vector_one<model::track_id>>,
                        [](model::track_id old) { //
                            SPDLOG_INFO("converting model::track_id");
                            return format::track_id{old.tid * 100};
                        })

    );
    const auto format_load_archives =
        immer::archive::transform_save_archive(model_archives, map);
    (void) format_load_archives;

    // Z<decltype(model_load_archives)> qwe;
    // REQUIRE(json_str == "");

    // const auto map

    // const auto format_archives = model_archives.transform(map);

    // {
    //     const auto loaded =
    //         immer::archive::from_json_with_auto_archive<format::history>(
    //             json_str, format_names);
    //     const auto [json_str2, archives] =
    //         immer::archive::to_json_with_auto_archive(loaded, format_names);
    //     REQUIRE(json_str == json_str2);
    // }
}

namespace {
struct two_vectors
{
    BOOST_HANA_DEFINE_STRUCT(two_vectors, //
                             (vector_one<int>, ints),
                             (vector_one<std::string>, strings));

    DEFINE_OPERATIONS(two_vectors);
};
} // namespace

TEST_CASE("Not every type is converted")
{
    const auto names = immer::archive::get_archives_for_types(
        hana::tuple_t<two_vectors>, hana::make_map());
    const auto [json_str, archives] =
        immer::archive::to_json_with_auto_archive(two_vectors{}, names);

    const auto map =
        hana::make_map(hana::make_pair(hana::type_c<vector_one<int>>,
                                       [](int old) { //
                                           return double{};
                                       })

        );
    const auto format_load_archives =
        immer::archive::transform_save_archive(archives, map);
    (void) format_load_archives;
}
