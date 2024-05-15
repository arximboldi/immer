#pragma once

#include <immer/extra/archive/alias.hpp>

#include <immer/array.hpp>
#include <immer/map.hpp>

#include <cereal/types/utility.hpp>

namespace immer::archive {

struct node_id_tag;
using node_id = type_alias<std::size_t, node_id_tag>;

struct container_id_tag;
using container_id = type_alias<std::size_t, container_id_tag>;

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

    friend bool operator==(const values_load& left, const values_load& right)
    {
        return left.data == right.data;
    }
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

template <class T, class F>
auto transform(const values_load<T>& values, F&& func)
{
    using U   = std::decay_t<decltype(func(std::declval<T>()))>;
    auto data = std::vector<U>{};
    for (const auto& item : values.data) {
        data.push_back(func(item));
    }
    return values_load<U>{immer::array<U>{data.begin(), data.end()}};
}

template <class T, class F>
auto transform(const immer::map<node_id, values_load<T>>& map, F&& func)
{
    using U     = std::decay_t<decltype(func(std::declval<T>()))>;
    auto result = immer::map<node_id, values_load<U>>{};
    for (const auto& [key, value] : map) {
        result = std::move(result).set(key, transform(value, func));
    }
    return result;
}

} // namespace immer::archive
