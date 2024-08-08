#pragma once

#include <immer/extra/persist/detail/alias.hpp>

#include <immer/array.hpp>
#include <immer/map.hpp>

#include <cereal/types/utility.hpp>

namespace immer::persist {

struct node_id_tag;
using node_id = detail::type_alias<std::size_t, node_id_tag>;

struct container_id_tag;
using container_id = detail::type_alias<std::size_t, container_id_tag>;

template <class T>
struct values_save
{
    const T* begin = nullptr;
    const T* end   = nullptr;

    friend bool operator==(const values_save& left,
                           const values_save& right) = default;
};

template <class T>
struct values_load
{
    immer::array<T> data;

    values_load() = default;

    values_load(immer::array<T> data_)
        : data{std::move(data_)}
    {
    }

    values_load(const values_save<T>& save)
        : data{save.begin, save.end}
    {
    }

    friend bool operator==(const values_load& left,
                           const values_load& right) = default;
};

template <class Archive, class T>
void save(Archive& ar, const values_save<T>& value)
{
    ar(cereal::make_size_tag(
        static_cast<cereal::size_type>(value.end - value.begin)));
    for (auto p = value.begin; p != value.end; ++p) {
        ar(*p);
    }
}

template <class Archive, class T>
void load(Archive& ar, values_load<T>& m)
{
    cereal::size_type size;
    ar(cereal::make_size_tag(size));

    for (auto i = cereal::size_type{}; i < size; ++i) {
        T x;
        ar(x);
        m.data = std::move(m.data).push_back(std::move(x));
    }
}

} // namespace immer::persist
