#include <fuzzer/fuzzer_input.hpp>

#include <immer/extra/persist/detail/champ/champ.hpp>

#include <array>
#include <bitset>

namespace {

struct broken_hash
{
    std::size_t operator()(std::size_t map_key) const { return map_key & ~15; }
};

const auto into_set = [](const auto& set) {
    using T     = typename std::decay_t<decltype(set)>::value_type;
    auto result = immer::set<T>{};
    for (const auto& item : set) {
        result = std::move(result).insert(item);
    }
    return result;
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size)
{
    constexpr auto var_count = 4;

    using set_t = immer::set<std::size_t, broken_hash>;

    auto vars = std::array<set_t, var_count>{};

    auto is_valid_var = [&](auto idx) { return idx >= 0 && idx < var_count; };

    return fuzzer_input{data, size}.run([&](auto& in) {
        enum ops
        {
            op_insert,
            op_erase,
            op_insert_move,
            op_erase_move,
            op_iterate,
            op_check,
        };
        auto src = read<char>(in, is_valid_var);
        auto dst = read<char>(in, is_valid_var);
        switch (read<char>(in)) {
        case op_insert: {
            auto value = read<size_t>(in);
            vars[dst]  = vars[src].insert(value);
            break;
        }
        case op_erase: {
            auto value = read<size_t>(in);
            vars[dst]  = vars[src].erase(value);
            break;
        }
        case op_insert_move: {
            auto value = read<size_t>(in);
            vars[dst]  = std::move(vars[src]).insert(value);
            break;
        }
        case op_erase_move: {
            auto value = read<size_t>(in);
            vars[dst]  = vars[src].erase(value);
            break;
        }
        case op_iterate: {
            auto srcv = vars[src];
            for (const auto& v : srcv) {
                vars[dst] = vars[dst].insert(v);
            }
            break;
        }
        case op_check: {
            const auto bitset = std::bitset<var_count>(read<size_t>(in));
            auto ids =
                std::vector<std::pair<std::size_t, immer::persist::node_id>>{};
            auto pool = immer::persist::champ::container_output_pool<set_t>{};
            for (auto index = std::size_t{}; index < var_count; ++index) {
                if (bitset[index]) {
                    auto set_id = immer::persist::node_id{};
                    std::tie(pool, set_id) =
                        immer::persist::champ::add_to_pool(vars[index], pool);
                    ids.emplace_back(index, set_id);
                }
            }

            auto loader = immer::persist::champ::container_loader<set_t>{
                to_input_pool(pool)};

            for (const auto& [index, set_id] : ids) {
                const auto& set   = vars[index];
                const auto loaded = loader.load(set_id);
                assert(into_set(loaded) == into_set(set));
                assert(into_set(set).size() == set.size());
                for (const auto& item : set) {
                    // This is the only thing that actually breaks if the hash
                    // of the loaded set is not the same as the hash function of
                    // the serialized set.
                    assert(loaded.count(item));
                }
                for (const auto& item : loaded) {
                    assert(set.count(item));
                }
            }

            break;
        }
        default:
            break;
        };
        return true;
    });
}
