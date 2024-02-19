#pragma once

#include <immer/experimental/immer-archive/alias.hpp>

#include <immer/array.hpp>

#include <cereal/types/utility.hpp>

namespace immer_archive {

struct node_id_tag;
using node_id = type_alias<std::size_t, node_id_tag>;

struct container_id_tag;
using container_id = type_alias<std::size_t, container_id_tag>;

template <class T>
struct values_save
{
    const T* begin = nullptr;
    const T* end   = nullptr;

    auto tie() const { return std::tie(begin, end); }

    friend bool operator==(const values_save& left, const values_save& right)
    {
        return left.tie() == right.tie();
    }
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

} // namespace immer_archive
