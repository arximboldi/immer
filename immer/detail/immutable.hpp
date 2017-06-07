#pragma once

#include <functional>

namespace immer {
namespace detail {

template <typename Container, typename Key, typename Updater>
static inline auto updateIn(Container const & container, Key const & key, Updater const & updater)
{
    return container.update(key, updater);
}

template <typename Container, typename Key, typename ... Rest>
static inline auto updateIn(Container const & container, Key const & key, Rest const & ... rest)
{
    return container.update(key, [&](auto const & child) {
        return updateIn(child, rest ...);
    });
}

template <typename Container, typename Key, typename Value>
static inline auto setIn(Container const & container, Key const & key, Value const & value)
{
    return container.set(key, value);
}

template <typename Container, typename Key, typename ... Rest>
static inline auto setIn(Container const & container, Key const & key, Rest const & ... rest)
{
    return container.update(key, [&](auto const & child) {
        return setIn(child, rest ...);
    });
}

}
}
