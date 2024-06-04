#pragma once

#include <immer/extra/persist/errors.hpp>
#include <immer/extra/persist/json/json_immer.hpp>
#include <immer/extra/persist/json/pools.hpp>
#include <immer/extra/persist/traits.hpp>

#include <fmt/format.h>

#include <boost/core/demangle.hpp>

namespace immer::persist {

namespace detail {
template <typename, typename = void>
constexpr bool is_iterable{};

template <typename T>
constexpr bool is_iterable<T,
                           std::void_t<decltype(std::declval<T>().begin()),
                                       decltype(std::declval<T>().end())>> =
    true;

template <class T>
auto get_iterator_type()
{
    if constexpr (is_iterable<T>) {
        return T{}.begin();
    } else {
        return;
    }
}

} // namespace detail

template <class Container>
struct persistable
{
    Container container;

    persistable() = default;

    persistable(std::initializer_list<typename Container::value_type> values)
        : container{std::move(values)}
    {
    }

    persistable(Container container_)
        : container{std::move(container_)}
    {
    }

    friend bool operator==(const persistable& left,
                           const persistable& right) = default;

    // template <std::enable_if_t<detail::is_iterable<Container>, bool> = true>
    // friend auto begin(const persistable& value)
    // {
    //     return value.container.begin();
    // }

    // friend std::enable_if_t<detail::is_iterable<Container>,
    //                         decltype(std::declval<Container>().end())>
    // end(const persistable& value)
    // {
    //     return value.container.end();
    // }
};

template <class Previous,
          class Storage,
          class WrapFn,
          class PoolNameFn,
          class Container>
auto save_minimal(const json_immer_output_archive<Previous,
                                                  detail::output_pools<Storage>,
                                                  WrapFn,
                                                  PoolNameFn>& ar,
                  const persistable<Container>& value)
{
    auto& pool =
        const_cast<json_immer_output_archive<Previous,
                                             detail::output_pools<Storage>,
                                             WrapFn,
                                             PoolNameFn>&>(ar)
            .get_output_pools()
            .template get_output_pool<Container>();
    auto [pool2, id] = add_to_pool(value.container, std::move(pool));
    pool             = std::move(pool2);
    return id.value;
}

// This function must exist because cereal does some checks and it's not
// possible to have only load_minimal for a type without having save_minimal.
template <class Previous,
          class Storage,
          class WrapFn,
          class PoolNameFn,
          class Container>
auto save_minimal(const json_immer_output_archive<Previous,
                                                  detail::input_pools<Storage>,
                                                  WrapFn,
                                                  PoolNameFn>& ar,
                  const persistable<Container>& value) ->
    typename container_traits<Container>::container_id::rep_t
{
    throw std::logic_error{"Should never be called"};
}

template <class Previous,
          class Pools,
          class WrapFn,
          class PoolNameFn,
          class Container>
void load_minimal(
    const json_immer_input_archive<Previous, Pools, WrapFn, PoolNameFn>& ar,
    persistable<Container>& value,
    const typename container_traits<Container>::container_id::rep_t& id)
{
    auto& loader =
        const_cast<
            json_immer_input_archive<Previous, Pools, WrapFn, PoolNameFn>&>(ar)
            .template get_loader<Container>();

    // Have to be specific because for vectors container_id is different from
    // node_id, but for hash-based containers, a container is identified just by
    // its root node.
    using container_id_ = typename container_traits<Container>::container_id;

    try {
        value.container = loader.load(container_id_{id});
    } catch (const pool_exception& ex) {
        if (!ar.ignore_pool_exceptions) {
            throw ::cereal::Exception{fmt::format(
                "Failed to load a container ID {} from the pool of {}: {}",
                id,
                boost::core::demangle(typeid(Container).name()),
                ex.what())};
        }
    }
}

// This function must exist because cereal does some checks and it's not
// possible to have only load_minimal for a type without having save_minimal.
template <class Archive, class Container>
auto save_minimal(const Archive& ar, const persistable<Container>& value) ->
    typename container_traits<Container>::container_id::rep_t
{
    throw std::logic_error{
        "Should never be called. save_minimal(const Archive& ar..."};
}

template <class Archive, class Container>
void load_minimal(
    const Archive& ar,
    persistable<Container>& value,
    const typename container_traits<Container>::container_id::rep_t& id)
{
    // This one is actually called while loading with not-yet-fully-loaded
    // pool.
}

} // namespace immer::persist
