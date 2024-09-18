#include <catch2/catch_test_macros.hpp>

#include <xxhash.h>

namespace {

template <class rep_t = std::uint16_t>
struct my_hash_t
{
    rep_t data{};

    my_hash_t() = default;

    constexpr explicit my_hash_t(rep_t data_)
        : data{data_}
    {
    }

    explicit operator bool() { return data; }

    my_hash_t& operator=(rep_t data_)
    {
        data = data_;
        return *this;
    }

    friend bool operator==(const my_hash_t& left, const my_hash_t& right)
    {
        return left.data == right.data;
    }

    template <class T>
    friend constexpr auto operator<<(my_hash_t left, const T& other)
    {
        auto result = static_cast<std::uint16_t>(left.data << other);
        return my_hash_t<std::uint16_t>{result};
    }

    // NOTE: Here we escape from the my_hash_t
    template <class T>
    friend constexpr auto operator<<(const T& other, my_hash_t right)
    {
        return other << right.data;
    }

    // NOTE: Here we escape from the my_hash_t
    template <class T>
    friend constexpr auto operator>>(my_hash_t left, const T& other)
    {
        return left.data >> other;
    }

    template <class T>
    friend constexpr auto operator-(my_hash_t left, const T& other)
    {
        auto result = static_cast<std::uint16_t>(left.data - other);
        return my_hash_t<std::uint16_t>{result};
    }

    template <class T>
    friend constexpr auto operator&(my_hash_t left, const my_hash_t<T>& other)
    {
        auto result = static_cast<std::uint16_t>(left.data & other.data);
        return my_hash_t<std::uint16_t>{result};
    }

    template <class T>
    friend constexpr std::enable_if_t<std::is_integral_v<T>,
                                      my_hash_t<std::uint16_t>>
    operator&(const T& other, my_hash_t right)
    {
        auto result = static_cast<std::uint16_t>(other & right.data);
        return my_hash_t<std::uint16_t>{result};
    }
};

struct small_hash
{
    my_hash_t<> operator()(const std::string& str) const
    {
        return my_hash_t{
            static_cast<std::uint16_t>(XXH3_64bits(str.c_str(), str.size()))};
    }
};

auto gen_strings()
{
    auto result = std::vector<std::string>{};
    for (int i = 0; i < 1000; ++i) {
        result.push_back(fmt::format("__{}__", i));
    }
    return result;
}

struct table_item
{
    std::string id;
    int data;
};

} // namespace

TEST_CASE("Test hash size for set")
{
    const auto strings = gen_strings();
    REQUIRE(strings.size() == 1000);

    const auto set =
        immer::set<std::string, small_hash>{strings.begin(), strings.end()};
    REQUIRE(set.size() == strings.size());
    for (const auto& item : strings) {
        REQUIRE(set.count(item));
    }
}

TEST_CASE("Test hash size for map")
{
    const auto strings = gen_strings();
    REQUIRE(strings.size() == 1000);

    auto map = immer::map<std::string, int, small_hash>{};
    for (const auto& [index, item] : boost::adaptors::index(strings)) {
        map = std::move(map).set(item, index);
    }

    REQUIRE(map.size() == strings.size());
    for (const auto& [index, item] : boost::adaptors::index(strings)) {
        REQUIRE(map[item] == index);
    }
}

TEST_CASE("Test hash size for table")
{
    const auto strings = gen_strings();
    REQUIRE(strings.size() == 1000);

    auto table = immer::table<table_item, immer::table_key_fn, small_hash>{};
    for (const auto& [index, item] : boost::adaptors::index(strings)) {
        table = std::move(table).insert(table_item{
            .id   = item,
            .data = static_cast<int>(index),
        });
    }

    REQUIRE(table.size() == strings.size());
    for (const auto& [index, item] : boost::adaptors::index(strings)) {
        REQUIRE(table[item].data == index);
    }
}
