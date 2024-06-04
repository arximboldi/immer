#pragma once

#include <boost/core/demangle.hpp>
#include <boost/hana.hpp>

namespace immer::persist {

struct get_demangled_name_fn
{
    template <class T>
    auto operator()(const T&) const
    {
        return boost::core::demangle(typeid(std::decay_t<T>).name());
    }
};

template <class Map>
struct name_from_map_fn
{
    auto operator()(const auto& container) const
    {
        return Map{}[boost::hana::typeid_(container)].c_str();
    }
};

} // namespace immer::persist
