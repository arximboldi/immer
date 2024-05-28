#pragma once

#include <cereal/archives/json.hpp>

/**
 * This cereal archive automatically wraps values into
 * immer::persist::persistable whenever possible and forwards into the
 * "previous" archive (which normally would be
 * `json_immer_[out|in]put_archive`).
 */

namespace immer::persist {

/**
 * Adapted from cereal/archives/adapters.hpp
 */

template <class Previous, class WrapF>
class json_immer_auto_output_archive
    : public cereal::OutputArchive<
          json_immer_auto_output_archive<Previous, WrapF>>
    , public cereal::traits::TextArchive
{
public:
    json_immer_auto_output_archive(Previous& previous_, WrapF wrapF_)
        : cereal::OutputArchive<
              json_immer_auto_output_archive<Previous, WrapF>>{this}
        , previous{previous_}
        , wrapF{std::move(wrapF_)}
    {
    }

    void finalize()
    {
        auto& self = *this;
        self(cereal::make_nvp("pools", previous.get_output_pools()));
    }

    template <class T>
    friend void prologue(json_immer_auto_output_archive& ar, T&& v)
    {
        using cereal::prologue;
        prologue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void epilogue(json_immer_auto_output_archive& ar, T&& v)
    {
        using cereal::epilogue;
        epilogue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_auto_output_archive& ar,
                                          cereal::NameValuePair<T> const& t)
    {
        ar.previous.setNextName(t.name);
        ar(ar.wrapF(t.value));
    }

    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_auto_output_archive& ar,
                                          std::nullptr_t const& t)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, t);
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_auto_output_archive& ar,
                                          T const& t)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, t);
    }

    template <class CharT, class Traits, class Alloc>
    friend void CEREAL_SAVE_FUNCTION_NAME(
        json_immer_auto_output_archive& ar,
        std::basic_string<CharT, Traits, Alloc> const& str)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, str);
    }

    template <class T>
    friend void CEREAL_SAVE_FUNCTION_NAME(json_immer_auto_output_archive& ar,
                                          cereal::SizeTag<T> const& v)
    {
        using cereal::CEREAL_SAVE_FUNCTION_NAME;
        CEREAL_SAVE_FUNCTION_NAME(ar.previous, v);
    }

    Previous& previous;
    WrapF wrapF;
};

template <class Previous, class WrapF>
class json_immer_auto_input_archive
    : public cereal::InputArchive<
          json_immer_auto_input_archive<Previous, WrapF>>
    , public cereal::traits::TextArchive
{
public:
    explicit json_immer_auto_input_archive(Previous& previous_, WrapF wrapF_)
        : cereal::InputArchive<
              json_immer_auto_input_archive<Previous, WrapF>>{this}
        , previous{previous_}
        , wrapF{std::move(wrapF_)}
    {
    }

    template <class T>
    void loadValue(T& value)
    {
        // if value is a ref to something non-immer-persistable, just return the
        // reference to it directly. Otherwise, wrap it in persistable.
        auto& wrapped = wrapF(value);
        previous.loadValue(wrapped);
    }

    template <class T>
    friend void prologue(json_immer_auto_input_archive& ar, T&& v)
    {
        using cereal::prologue;
        prologue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void epilogue(json_immer_auto_input_archive& ar, T&& v)
    {
        using cereal::epilogue;
        epilogue(ar.previous, std::forward<T>(v));
    }

    template <class T>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_auto_input_archive& ar,
                                          cereal::NameValuePair<T>& t)
    {
        ar.previous.setNextName(t.name);
        auto&& wrapped = ar.wrapF(t.value);
        ar(wrapped);
    }

    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_auto_input_archive& ar,
                                          std::nullptr_t& t)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, t);
    }

    template <class T,
              cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
                  cereal::traits::sfinae>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_auto_input_archive& ar,
                                          T& t)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, t);
    }

    template <class CharT, class Traits, class Alloc>
    friend void
    CEREAL_LOAD_FUNCTION_NAME(json_immer_auto_input_archive& ar,
                              std::basic_string<CharT, Traits, Alloc>& str)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, str);
    }

    template <class T>
    friend void CEREAL_LOAD_FUNCTION_NAME(json_immer_auto_input_archive& ar,
                                          cereal::SizeTag<T>& st)
    {
        using cereal::CEREAL_LOAD_FUNCTION_NAME;
        CEREAL_LOAD_FUNCTION_NAME(ar.previous, st);
    }

    Previous& previous;
    WrapF wrapF;
};

} // namespace immer::persist

// tie input and output archives together
namespace cereal::traits::detail {

template <class Previous, class WrapF>
struct get_output_from_input<
    immer::persist::json_immer_auto_input_archive<Previous, WrapF>>
{
    using type =
        immer::persist::json_immer_auto_output_archive<Previous, WrapF>;
};
template <class Previous, class WrapF>
struct get_input_from_output<
    immer::persist::json_immer_auto_output_archive<Previous, WrapF>>
{
    using type = immer::persist::json_immer_auto_input_archive<Previous, WrapF>;
};

} // namespace cereal::traits::detail
