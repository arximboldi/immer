#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

#include <immer/extra/persist/cereal/load.hpp>
#include <immer/extra/persist/cereal/save.hpp>
#include <immer/extra/persist/xxhash/xxhash.hpp>

#include <immer/extra/persist/transform.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

#include <boost/hana.hpp>

#include <algorithm>
#include <iostream>
#include <ranges>

template <class T>
using vector = immer::vector<T, immer::default_memory_policy, 1, 1>;

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

struct document
{
    vector<std::string> data;
    friend bool operator==(const document&, const document&) = default;
    friend bool operator!=(const document&, const document&) = default;
};

BOOST_HANA_ADAPT_STRUCT(document, data);

struct document_v2
{
    vector<char> data;
    friend bool operator==(const document_v2&, const document_v2&) = default;
    friend bool operator!=(const document_v2&, const document_v2&) = default;
};

BOOST_HANA_ADAPT_STRUCT(document_v2, data);

template <typename Archive, typename T>
auto serialize(Archive& ar, T& v)
    -> std::enable_if_t<boost::hana::Struct<T>::value>
{
    boost::hana::for_each(boost::hana::keys(v), [&](auto&& k) {
        ar(cereal::make_nvp(k.c_str(), boost::hana::at_key(v, k)));
    });
}

std::string to_uppercase(std::string input)
{
    std::transform(input.begin(),
                   input.end(),
                   input.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return input;
}

auto transform_vector(auto vector, auto func)
{
    auto transient = decltype(vector.transient()){};
    std::transform(
        vector.begin(), vector.end(), std::back_inserter(transient), func);
    return transient.persistent();
}

int main()
{
    const auto v1 = vector<std::string>{"a", "b", "c", "d"};
    const auto v2 = v1.push_back("e").push_back("f");
    const auto v3 = v2;

    {
        auto x1 = transform_vector(v1, [](auto s) { return to_uppercase(s); });
        auto x2 = transform_vector(v2, [](auto s) { return to_uppercase(s); });
        auto x3 = transform_vector(v3, [](auto s) { return to_uppercase(s); });
        assert(x1.size() == v1.size());
        assert(x2.size() == v2.size());
        assert(x3.size() == v3.size());
    }

    const auto history = vector<document>{{v1}, {v2}, {v3}};

    namespace imp = immer::persist;

    {
        auto ar = cereal::JSONOutputArchive{std::cout};
        ar(history);
    }

    // serialization
    {
        std::cout << to_json(history) << "\n";

        auto policy = imp::hana_struct_auto_member_name_policy(history);
        auto str    = imp::cereal_save_with_pools(history, policy);

        std::cout << str << "\n";

        auto history_2 =
            imp::cereal_load_with_pools<decltype(history)>(str, policy);
        std::cout << "comparison = " << (history == history_2) << std::endl;
    }

    // conversions
    {
        namespace hana = boost::hana;

        const auto xform = hana::make_map(hana::make_pair(
            hana::type_c<vector<std::string>>,
            [](std::string v) -> std::string { return to_uppercase(v); }));

        const auto pool = imp::get_output_pools(history);
        auto xf_pool    = imp::transform_output_pool(pool, xform);

        auto view = history | std::views::transform([&](document x) {
                        auto data =
                            imp::convert_container(pool, xf_pool, x.data);
                        return document{data};
                    });

        auto xformed_history = vector<document>{view.begin(), view.end()};

        auto policy = imp::hana_struct_auto_member_name_policy(xformed_history);
        imp::cereal_save_with_pools<cereal::JSONOutputArchive>(
            std::cout, xformed_history, policy);
    }

    // conversions
    {
        namespace hana = boost::hana;

        const auto xform = hana::make_map(hana::make_pair(
            hana::type_c<vector<std::string>>,
            [](std::string v) -> char { return v.empty() ? '\0' : v[0]; }));

        const auto pool = imp::get_output_pools(history);
        auto xf_pool    = imp::transform_output_pool(pool, xform);

        auto view = history | std::views::transform([&](document x) {
                        auto data =
                            imp::convert_container(pool, xf_pool, x.data);
                        return document_v2{data};
                    });

        auto xformed_history = vector<document_v2>{view.begin(), view.end()};

        auto policy = imp::hana_struct_auto_member_name_policy(xformed_history);
        std::cout << imp::cereal_save_with_pools(xformed_history, policy)
                  << "\n";
    }

    {
    }
}
