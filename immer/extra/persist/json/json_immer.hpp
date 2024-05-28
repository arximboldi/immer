#pragma once

#include <cereal/archives/json.hpp>

#include <boost/hana/functional/id.hpp>

/**
 * Special types of archives, working with JSON, that support providing extra
 * context (Pools) to serialize immer data structures.
 */

namespace immer::persist {

struct blackhole_output_archive
{
    void setNextName(const char* name) {}

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          cereal::NameValuePair<T> const& t)
    {
    }

    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          std::nullptr_t const& t)
    {
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          T const& t)
    {
    }

    template <class CharT, class Traits, class Alloc>
    friend void CEREAL_SAVE_FUNCTION_NAME(
        blackhole_output_archive& ar,
        std::basic_string<CharT, Traits, Alloc> const& str)
    {
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(blackhole_output_archive& ar,
                                          cereal::SizeTag<T> const& v)
    {
    }
};

/**
 * Adapted from cereal/archives/adapters.hpp
 */

template <class Previous, class Pools, class WrapF = boost::hana::id_t>
class json_immer_output_archive
    : public cereal::OutputArchive<
          json_immer_output_archive<Previous, Pools, WrapF>>
    , public cereal::traits::TextArchive
{
public:
    template <class... Args>
    explicit json_immer_output_archive(Args&&... args)
        requires std::is_same_v<WrapF, boost::hana::id_t>
        : cereal::OutputArchive<json_immer_output_archive>{this}
        , previous{std::forward<Args>(args)...}
    {
    }

    template <class... Args>
    json_immer_output_archive(Pools pools_, Args&&... args)
        requires std::is_same_v<WrapF, boost::hana::id_t>
        : cereal::OutputArchive<json_immer_output_archive>{this}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    template <class... Args>
    json_immer_output_archive(Pools pools_, WrapF wrap_, Args&&... args)
        : cereal::OutputArchive<json_immer_output_archive>{this}
        , wrap{std::move(wrap_)}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    void setNextName(const char* name) { previous.setNextName(name); }

    Pools& get_output_pools() & { return pools; }
    Pools&& get_output_pools() && { return std::move(pools); }

    void finalize()
    {
        auto& self = *this;
        self(CEREAL_NVP(pools));
    }

    template <class T>
    friend void prologue(json_immer_output_archive& ar, T&& v)
    {
        using cereal::prologue;
        prologue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void epilogue(json_immer_output_archive& ar, T&& v)
    {
        using cereal::epilogue;
        epilogue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          cereal::NameValuePair<T> const& t)
    {
        ar.previous.setNextName(t.name);
        ar(ar.wrap(t.value));
    }

    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          std::nullptr_t const& t)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, t);
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          T const& t)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, t);
    }

    template <class CharT, class Traits, class Alloc>
    friend void CEREAL_SAVE_FUNCTION_NAME(
        json_immer_output_archive& ar,
        std::basic_string<CharT, Traits, Alloc> const& str)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, str);
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive& ar,
                                          cereal::SizeTag<T> const& v)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, v);
    }

private:
    WrapF wrap;
    Previous previous;
    Pools pools;
};

template <class Previous, class Pools, class WrapF = boost::hana::id_t>
class json_immer_input_archive
    : public cereal::InputArchive<
          json_immer_input_archive<Previous, Pools, WrapF>>
    , public cereal::traits::TextArchive
{
public:
    template <class... Args>
    json_immer_input_archive(Pools pools_, Args&&... args)
        requires std::is_same_v<WrapF, boost::hana::id_t>
        : cereal::InputArchive<json_immer_input_archive>{this}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    template <class... Args>
    json_immer_input_archive(Pools pools_, WrapF wrap_, Args&&... args)
        : cereal::InputArchive<json_immer_input_archive>{this}
        , wrap{std::move(wrap_)}
        , previous{std::forward<Args>(args)...}
        , pools{std::move(pools_)}
    {
    }

    void setNextName(const char* name) { previous.setNextName(name); }

    Pools& get_input_pools() { return pools; }
    const Pools& get_input_pools() const { return pools; }

    template <class T>
    friend void prologue(json_immer_input_archive& ar, T&& v)
    {
        using cereal::prologue;
        prologue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void epilogue(json_immer_input_archive& ar, T&& v)
    {
        using cereal::epilogue;
        epilogue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                                          cereal::NameValuePair<T>& t)
    {
        ar.previous.setNextName(t.name);
        auto&& wrapped = ar.wrap(t.value);
        ar(wrapped);
    }

    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                                          std::nullptr_t& t)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, t);
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar, T& t)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, t);
    }

    template <class CharT, class Traits, class Alloc>
    friend void
    CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                              std::basic_string<CharT, Traits, Alloc>& str)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, str);
    }

    template <class T>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive& ar,
                                          cereal::SizeTag<T>& st)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, st);
    }

private:
    WrapF wrap;
    Previous previous;
    Pools pools;
};

} // namespace immer::persist

// tie input and output archives together
namespace cereal {
namespace traits {
namespace detail {
template <class Previous, class Pools, class WrapF>
struct get_output_from_input<
    immer::persist::json_immer_input_archive<Previous, Pools, WrapF>>
{
    using type =
        immer::persist::json_immer_output_archive<Previous, Pools, WrapF>;
};
template <class Previous, class Pools, class WrapF>
struct get_input_from_output<
    immer::persist::json_immer_output_archive<Previous, Pools, WrapF>>
{
    using type =
        immer::persist::json_immer_input_archive<Previous, Pools, WrapF>;
};
} // namespace detail
} // namespace traits
} // namespace cereal
