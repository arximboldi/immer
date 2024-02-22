#pragma once

#include <immer/extra/archive/rbts/archive.hpp>
#include <immer/extra/archive/rbts/load.hpp>

#include <sstream>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace test {

template <class T>
using vector_one =
    immer::vector<T, immer::default_memory_policy, immer::default_bits, 1>;

template <class T>
using flex_vector_one =
    immer::flex_vector<T, immer::default_memory_policy, immer::default_bits, 1>;

using example_vector      = vector_one<int>;
using example_flex_vector = flex_vector_one<int>;

using example_archive_save =
    decltype(immer::archive::rbts::make_save_archive_for(example_vector{}));
using example_loader = immer::archive::rbts::
    loader<int, immer::default_memory_policy, immer::default_bits, 1>;

inline auto gen(auto init, int count)
{
    for (int i = 0; i < count; ++i) {
        init = std::move(init).push_back(i);
    }
    return init;
}

template <typename T>
std::string to_json(const T& serializable)
{
    auto os = std::ostringstream{};
    {
        auto ar = cereal::JSONOutputArchive{os};
        ar(serializable);
    }
    return os.str();
}

template <typename T>
T from_json(std::string input)
{
    auto is = std::istringstream{input};
    auto ar = cereal::JSONInputArchive{is};
    auto r  = T{};
    ar(r);
    return r;
}

struct test_value
{
    std::size_t id;
    std::string value;

    auto tie() const { return std::tie(id, value); }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(value));
    }

    friend bool operator==(const test_value& left, const test_value& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const test_value& value)
    {
        return s << fmt::format("({}, {})", value.id, value.value);
    }
};

struct old_type
{
    std::string id;
    int data;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(data));
    }
};

struct new_type
{
    std::string id;
    int data;
    std::string data2;

    auto tie() const { return std::tie(id, data, data2); }

    friend bool operator==(const new_type& left, const new_type& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(data), CEREAL_NVP(data2));
    }
};

inline auto convert_old_type(const old_type& val)
{
    return new_type{
        .id    = val.id,
        .data  = val.data,
        .data2 = fmt::format("_{}_", val.data),
    };
}

} // namespace test

template <>
struct fmt::formatter<test::test_value> : ostream_formatter
{};
