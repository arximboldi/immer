#include <immer/vector.hpp>

#include <immer/extra/persist/cereal/save.hpp>
#include <immer/extra/persist/xxhash/xxhash.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

#include <boost/hana.hpp>

#include <iostream>

template <class T>
using vector =
    immer::vector<T, immer::default_memory_policy, immer::default_bits, 1>;

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
};

BOOST_HANA_ADAPT_STRUCT(document, data);

template <typename Archive, typename T>
auto serialize(Archive& ar, T& v)
    -> std::enable_if_t<boost::hana::Struct<T>::value>
{
    boost::hana::for_each(boost::hana::keys(v), [&](auto&& k) {
        ar(cereal::make_nvp(k.c_str(), boost::hana::at_key(v, k)));
    });
}

int main()
{
    const auto v1 = vector<std::string>{"a", "b", "c", "d"};
    const auto v2 = v1.push_back("e").push_back("f");
    const auto v3 = v2;

    const auto history = vector<document>{{v1}, {v2}, {v3}};
    std::cout << to_json(history) << "\n";

    namespace imp = immer::persist;

    auto policy = imp::hana_struct_auto_member_name_policy(history);
    std::cout << imp::cereal_save_with_pools(history, policy) << "\n";
}
