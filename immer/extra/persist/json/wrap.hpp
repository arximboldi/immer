#pragma once

#include <immer/extra/persist/common/type_traverse.hpp>
#include <immer/extra/persist/json/persistable.hpp>

#include <immer/extra/persist/box/pool.hpp>
#include <immer/extra/persist/champ/traits.hpp>
#include <immer/extra/persist/rbts/traits.hpp>

namespace immer::persist {

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
 * This wrapper is used to load a given container via persistable.
 */
template <class Container>
struct persistable_loader_wrapper
{
    Container& value;

    template <class Archive>
    typename container_traits<Container>::container_id::rep_t
    save_minimal(const Archive&) const
    {
        throw std::logic_error{
            "Should never be called. persistable_loader_wrapper::save_minimal"};
    }

    template <class Archive>
    void load_minimal(
        const Archive& ar,
        const typename container_traits<Container>::container_id::rep_t&
            container_id)
    {
        persistable<Container> arch;
        immer::persist::load_minimal(ar, arch, container_id);
        value = std::move(arch).container;
    }
};

constexpr auto is_persistable = boost::hana::is_valid(
    [](auto&& obj) ->
    typename container_traits<std::decay_t<decltype(obj)>>::output_pool_t {});

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

// We must not try to persist types that are actually the pool itself,
// for example, `immer::map<node_id, values_save<T>> leaves` etc.
constexpr auto exclude_internal_pool_types = [](auto wrap) {
    namespace hana = boost::hana;
    return make_conditional_func(hana::compose(hana::not_, is_auto_ignored),
                                 wrap);
};

constexpr auto to_persistable = [](const auto& x) {
    return persistable<std::decay_t<decltype(x)>>(x);
};

constexpr auto to_persistable_loader = [](auto& value) {
    using V = std::decay_t<decltype(value)>;
    return persistable_loader_wrapper<V>{value};
};

/**
 * This function will wrap a value in persistable if possible or will return a
 * reference to its argument.
 */
constexpr auto wrap_for_saving = exclude_internal_pool_types(
    make_conditional_func(is_persistable, to_persistable));

constexpr auto wrap_for_loading = exclude_internal_pool_types(
    make_conditional_func(is_persistable, to_persistable_loader));

/**
 * Generate a hana set of types of persistable members for the given type,
 * recursively. Example: [type_c<immer::map<K, V>>]
 */
template <class T>
auto get_pools_for_type()
{
    namespace hana     = boost::hana;
    auto all_types_set = util::get_inner_types(hana::type_c<T>);
    auto persistable =
        hana::filter(hana::to_tuple(all_types_set), [](auto type) {
            using Type = typename decltype(type)::type;
            return is_persistable(Type{});
        });
    return hana::to_set(persistable);
}

/**
 * Generate a hana map of persistable members for the given type, recursively.
 * Example:
 * [(type_c<immer::map<K, V>>, "tracks")]
 */
template <class T>
auto get_named_pools_for_type()
{
    namespace hana     = boost::hana;
    auto all_types_map = util::get_inner_types_map(hana::type_c<T>);
    auto persistable =
        hana::filter(hana::to_tuple(all_types_map), [](auto pair) {
            using Type = typename decltype(+hana::first(pair))::type;
            return is_persistable(Type{});
        });
    return hana::to_map(persistable);
}

} // namespace immer::persist
