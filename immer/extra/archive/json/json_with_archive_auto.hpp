#pragma once

#include <immer/extra/archive/json/archivable.hpp>
#include <immer/extra/archive/json/json_immer.hpp>
#include <immer/extra/archive/json/json_immer_auto.hpp>
#include <immer/extra/archive/json/json_with_archive.hpp>

#include <immer/extra/archive/box/archive.hpp>
#include <immer/extra/archive/champ/traits.hpp>
#include <immer/extra/archive/rbts/traits.hpp>

namespace immer::archive {

template <class T>
struct is_auto_ignored_type : boost::hana::false_
{};

template <class T>
struct is_auto_ignored_type<immer::map<node_id, values_save<T>>>
    : boost::hana::true_
{};

template <class T>
struct is_auto_ignored_type<immer::map<node_id, values_load<T>>>
    : boost::hana::true_
{};

template <>
struct is_auto_ignored_type<immer::map<node_id, rbts::inner_node>>
    : boost::hana::true_
{};

template <>
struct is_auto_ignored_type<immer::vector<node_id>> : boost::hana::true_
{};

template <>
struct is_auto_ignored_type<immer::vector<rbts::rbts_info>> : boost::hana::true_
{};

/**
 * This wrapper is used to load a given container via archivable.
 */
template <class Container>
struct archivable_loader_wrapper
{
    Container& value;

    template <class Archive>
    typename container_traits<Container>::container_id::rep_t
    save_minimal(const Archive&) const
    {
        throw std::logic_error{
            "Should never be called. archivable_loader_wrapper::save_minimal"};
    }

    template <class Archive>
    void load_minimal(
        const Archive& ar,
        const typename container_traits<Container>::container_id::rep_t&
            container_id)
    {
        archivable<Container> arch;
        immer::archive::load_minimal(ar.previous, arch, container_id);
        value = std::move(arch).container;
    }
};

constexpr auto is_archivable = boost::hana::is_valid(
    [](auto&& obj) ->
    typename container_traits<std::decay_t<decltype(obj)>>::save_archive_t {});

constexpr auto is_auto_ignored = [](const auto& value) {
    return is_auto_ignored_type<std::decay_t<decltype(value)>>{};
};

/**
 * Make a function that operates conditionally on its single argument, based on
 * the given predicate. If the predicate is not satisfied, the function forwards
 * its argument unchanged.
 */
constexpr auto make_conditional_func = [](auto pred, auto func) {
    return [pred, func](auto&& value) -> decltype(auto) {
        return boost::hana::if_(pred(value), func, boost::hana::id)(
            std::forward<decltype(value)>(value));
    };
};

// We must not try to archive types that are actually the archive itself,
// for example, `immer::map<node_id, values_save<T>> leaves` etc.
constexpr auto exclude_internal_archive_types = [](auto wrap) {
    namespace hana = boost::hana;
    return make_conditional_func(hana::compose(hana::not_, is_auto_ignored),
                                 wrap);
};

constexpr auto to_archivable = [](const auto& x) {
    return archivable<std::decay_t<decltype(x)>>(x);
};

constexpr auto to_archivable_loader = [](auto& value) {
    using V = std::decay_t<decltype(value)>;
    return archivable_loader_wrapper<V>{value};
};

/**
 * This function will wrap a value in archivable if possible or will return a
 * reference to its argument.
 */
constexpr auto wrap_for_saving = exclude_internal_archive_types(
    make_conditional_func(is_archivable, to_archivable));

constexpr auto wrap_for_loading = exclude_internal_archive_types(
    make_conditional_func(is_archivable, to_archivable_loader));

/**
 * Generate a hana map of archivable members for a given type T.
 * Where key is a type (archivable immer container) and value is a name for that
 * archive (using the member name from the given struct).
 * Example: [(type_c<immer::map<K, V>>, "tracks")]
 */
template <class T>
auto get_auto_archives_types(const T& value)
{
    namespace hana = boost::hana;
    static_assert(hana::Struct<T>::value,
                  "get_auto_archives_types works only with types that "
                  "implement hana::Struct concept");

    constexpr auto is_archivable_key = [](auto key) {
        return is_archivable(hana::at_key(T{}, key));
    };

    // A list of pairs like (hana::type_c<Container>, "member_name")
    auto pairs = hana::transform(
        hana::filter(hana::keys(value), is_archivable_key), [&](auto key) {
            const auto& member = hana::at_key(value, key);
            return hana::make_pair(hana::typeid_(member), key);
        });

    return hana::unpack(pairs, hana::make_map);
}

/**
 * Generate a hana map of archivable members for the given types and apply
 * manual overrides for names.
 * manual_overrides is a map of manual overrides.
 */
auto get_archives_for_types(auto types,
                            auto manual_overrides = boost::hana::make_map())
{
    namespace hana = boost::hana;
    // Automatically generate names and archives
    const auto names =
        hana::fold_left(hana::transform(types,
                                        [](auto t) {
                                            using T =
                                                typename decltype(t)::type;
                                            return get_auto_archives_types(T{});
                                        }),
                        hana::make_map(),
                        hana::union_);
    // Apply the overrides
    return hana::union_(names, manual_overrides);
}

template <typename T, class ArchivesTypes>
auto to_json_with_auto_archive(const T& serializable,
                               const ArchivesTypes& archives_types)
{
    namespace hana = boost::hana;

    // In the future, wrap function may ignore certain user-provided types that
    // should not be archived.
    constexpr auto wrap = wrap_for_saving;
    static_assert(
        std::is_same_v<decltype(wrap(std::declval<const std::string&>())),
                       const std::string&>,
        "wrap must return a reference when it's not wrapping the type");
    static_assert(std::is_same_v<decltype(wrap(immer::vector<int>{})),
                                 archivable<immer::vector<int>>>,
                  "and a value when it's wrapping");

    using WrapF = std::decay_t<decltype(wrap)>;

    auto archives  = detail::generate_archives_save(archives_types);
    using Archives = std::decay_t<decltype(archives)>;

    const auto save_archive = [wrap](auto archives) {
        auto previous =
            json_immer_output_archive<blackhole_output_archive, Archives>{
                archives};
        auto ar = json_immer_auto_output_archive<decltype(previous), WrapF>{
            previous, wrap};

        ar(archives);
        return std::move(previous).get_output_archives();
    };

    auto os = std::ostringstream{};
    {
        auto previous =
            json_immer_output_archive<cereal::JSONOutputArchive, Archives>{os};
        auto ar = json_immer_auto_output_archive<decltype(previous), WrapF>{
            previous, wrap};
        // value0 because that's now cereal saves the unnamed object by default,
        // maybe change later.
        ar(cereal::make_nvp("value0", serializable));
        if constexpr (!is_archive_empty<Archives>()) {
            save_archives_impl(previous, save_archive);
            ar.finalize();
        }
        archives = std::move(previous).get_output_archives();
    }
    return std::make_pair(os.str(), std::move(archives));
}

// Same as to_json_with_auto_archive but we don't generate any JSON.
template <typename T, class ArchivesTypes>
auto get_auto_archive(const T& serializable,
                      const ArchivesTypes& archives_types)
{
    namespace hana = boost::hana;

    // In the future, wrap function may ignore certain user-provided types that
    // should not be archived.
    constexpr auto wrap = wrap_for_saving;
    static_assert(
        std::is_same_v<decltype(wrap(std::declval<const std::string&>())),
                       const std::string&>,
        "wrap must return a reference when it's not wrapping the type");
    static_assert(std::is_same_v<decltype(wrap(immer::vector<int>{})),
                                 archivable<immer::vector<int>>>,
                  "and a value when it's wrapping");

    using WrapF = std::decay_t<decltype(wrap)>;

    auto archives  = detail::generate_archives_save(archives_types);
    using Archives = std::decay_t<decltype(archives)>;

    const auto save_archive = [wrap](auto archives) {
        auto previous =
            json_immer_output_archive<blackhole_output_archive, Archives>{
                archives};
        auto ar = json_immer_auto_output_archive<decltype(previous), WrapF>{
            previous, wrap};
        ar(archives);
        return std::move(previous).get_output_archives();
    };

    {
        auto previous =
            json_immer_output_archive<blackhole_output_archive, Archives>{};
        auto ar = json_immer_auto_output_archive<decltype(previous), WrapF>{
            previous, wrap};
        // value0 because that's now cereal saves the unnamed object by default,
        // maybe change later.
        ar(cereal::make_nvp("value0", serializable));
        if constexpr (!is_archive_empty<Archives>()) {
            save_archives_impl(previous, save_archive);
            ar.finalize();
        }
        archives = std::move(previous).get_output_archives();
    }
    return archives;
}

template <typename Archives, class WrapF>
auto load_initial_auto_archives(std::istream& is, WrapF wrap)
{
    auto archives = Archives{};
    if constexpr (is_archive_empty<Archives>()) {
        return archives;
    }

    auto restore  = util::istream_snapshot{is};
    auto previous = cereal::JSONInputArchive{is};
    auto ar = json_immer_auto_input_archive<decltype(previous), WrapF>{previous,
                                                                       wrap};
    ar(CEREAL_NVP(archives));
    return archives;
}

constexpr auto reload_archive_auto = [](auto wrap) {
    return [wrap](std::istream& is,
                  auto archives,
                  bool ignore_archive_exceptions) {
        using Archives                     = std::decay_t<decltype(archives)>;
        using WrapF                        = std::decay_t<decltype(wrap)>;
        auto restore                       = util::istream_snapshot{is};
        archives.ignore_archive_exceptions = ignore_archive_exceptions;
        auto previous =
            json_immer_input_archive<cereal::JSONInputArchive, Archives>{
                std::move(archives), is};
        auto ar = json_immer_auto_input_archive<decltype(previous), WrapF>{
            previous, wrap};
        /**
         * NOTE: Critical to clear the archives before loading into it
         * again. I hit a bug when archives contained a vector and every
         * load would append to it, instead of replacing the contents.
         */
        archives = {};
        ar(CEREAL_NVP(archives));
        return archives;
    };
};

template <typename T, class ArchivesTypes>
T from_json_with_auto_archive(std::istream& is,
                              const ArchivesTypes& archives_types)
{
    namespace hana      = boost::hana;
    constexpr auto wrap = wrap_for_loading;
    using WrapF         = std::decay_t<decltype(wrap)>;

    using Archives =
        std::decay_t<decltype(detail::generate_archives_load(archives_types))>;

    auto archives =
        load_archives(is,
                      load_initial_auto_archives<Archives>(is, wrap),
                      reload_archive_auto(wrap));

    auto previous =
        json_immer_input_archive<cereal::JSONInputArchive, Archives>{
            std::move(archives), is};
    auto ar = json_immer_auto_input_archive<decltype(previous), WrapF>{previous,
                                                                       wrap};
    // value0 because that's now cereal saves the unnamed object by default,
    // maybe change later.
    auto value0 = T{};
    ar(CEREAL_NVP(value0));
    return value0;
}

template <typename T, class ArchivesTypes>
T from_json_with_auto_archive(const std::string& input,
                              const ArchivesTypes& archives_types)
{
    auto is = std::istringstream{input};
    return from_json_with_auto_archive<T>(is, archives_types);
}

template <typename T,
          typename OldType,
          typename ConversionsMap,
          class ArchivesTypes>
T from_json_with_auto_archive_with_conversion(
    std::istream& is,
    const ConversionsMap& map,
    const ArchivesTypes& archives_types)
{
    constexpr auto wrap = wrap_for_loading;
    using WrapF         = std::decay_t<decltype(wrap)>;

    // Load the archives part for the old type
    using OldArchives =
        std::decay_t<decltype(detail::generate_archives_load(archives_types))>;
    auto archives_old =
        load_archives(is,
                      load_initial_auto_archives<OldArchives>(is, wrap),
                      reload_archive_auto(wrap));

    auto archives  = archives_old.transform(map);
    using Archives = decltype(archives);

    auto previous =
        json_immer_input_archive<cereal::JSONInputArchive, Archives>{
            std::move(archives), is};
    auto ar = json_immer_auto_input_archive<decltype(previous), WrapF>{previous,
                                                                       wrap};
    auto r  = T{};
    ar(r);
    return r;
}

template <typename T,
          typename OldType,
          typename ConversionsMap,
          class ArchivesTypes>
T from_json_with_auto_archive_with_conversion(
    const std::string& input,
    const ConversionsMap& map,
    const ArchivesTypes& archives_types)
{
    auto is = std::istringstream{input};
    return from_json_with_auto_archive_with_conversion<T, OldType>(
        is, map, archives_types);
}

} // namespace immer::archive
