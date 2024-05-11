#pragma once

#include <cereal/archives/json.hpp>

/**
 * Special types of archives, working with JSON, that support providing extra
 * context (ImmerArchives) to serialize immer data structures.
 */

namespace immer::archive {

struct blackhole_output_archive
{
    void startNode() const {}
    void writeName() const {}
    void finishNode() const {}
    void setNextName(const char* name) const {}
    void makeArray() const {}

    template <class T>
    void saveValue(const T& value)
    {
    }
};

/**
 * Adapted from cereal/archives/adapters.hpp
 */

template <class Previous, class ImmerArchives>
class json_immer_output_archive
    : public cereal::OutputArchive<
          json_immer_output_archive<Previous, ImmerArchives>>
    , public cereal::traits::TextArchive
{
public:
    template <class... Args>
    explicit json_immer_output_archive(Args&&... args)
        : cereal::OutputArchive<
              json_immer_output_archive<Previous, ImmerArchives>>{this}
        , previous{std::forward<Args>(args)...}
    {
    }

    template <class... Args>
    json_immer_output_archive(ImmerArchives archives, Args&&... args)
        : cereal::OutputArchive<
              json_immer_output_archive<Previous, ImmerArchives>>{this}
        , previous{std::forward<Args>(args)...}
        , archives{std::move(archives)}
    {
    }

    ~json_immer_output_archive() {}

    void startNode() { previous.startNode(); }
    void writeName() { previous.writeName(); }
    void finishNode() { previous.finishNode(); }
    void setNextName(const char* name) { previous.setNextName(name); }
    void makeArray() { previous.makeArray(); }

    template <class T>
    void saveValue(const T& value)
    {
        previous.saveValue(value);
    }

    ImmerArchives& get_output_archives() & { return archives; }

    ImmerArchives&& get_output_archives() && { return std::move(archives); }

    void finalize()
    {
        auto& self = *this;
        self(CEREAL_NVP(archives));
    }

private:
    Previous previous;
    ImmerArchives archives;
};

template <class Previous, class ImmerArchives>
class json_immer_input_archive
    : public cereal::InputArchive<
          json_immer_input_archive<Previous, ImmerArchives>>
    , public cereal::traits::TextArchive
{
public:
    template <class... Args>
    json_immer_input_archive(ImmerArchives archives_, Args&&... args)
        : cereal::InputArchive<
              json_immer_input_archive<Previous, ImmerArchives>>{this}
        , previous{std::forward<Args>(args)...}
        , archives{std::move(archives_)}
    {
    }

    void startNode() { previous.startNode(); }
    void finishNode() { previous.finishNode(); }
    void setNextName(const char* name) { previous.setNextName(name); }
    void loadSize(cereal::size_type& size) { previous.loadSize(size); }
    bool hasName(const char* name) { return previous.hasName(name); }

    template <class T>
    void loadValue(T& value)
    {
        previous.loadValue(value);
    }

    ImmerArchives& get_input_archives() { return archives; }
    const ImmerArchives& get_input_archives() const { return archives; }

private:
    Previous previous;
    ImmerArchives archives;
};

/**
 * NOTE: All of the following is needed, ultimately, to enable specializing
 * serializing functions specifically for this type of archive that provides
 * access to the immer-related archive.
 *
 * template <class Previous ,class ImmerArchives, class T>
 * void load(json_immer_input_archive<Previous, ImmerArchives>& ar,
 *           vector_one_archivable<T>& value)
 */

// ######################################################################
// json_immer_{input/output}_archive prologue and epilogue functions
// ######################################################################

// ######################################################################
//! Prologue for NVPs for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class Previous, class ImmerArchives, class T>
inline void prologue(json_immer_output_archive<Previous, ImmerArchives>&,
                     cereal::NameValuePair<T> const&)
{
}

//! Prologue for NVPs for JSON archives
template <class Previous, class ImmerArchives, class T>
inline void prologue(json_immer_input_archive<Previous, ImmerArchives>&,
                     cereal::NameValuePair<T> const&)
{
}

// ######################################################################
//! Epilogue for NVPs for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class Previous, class ImmerArchives, class T>
inline void epilogue(json_immer_output_archive<Previous, ImmerArchives>&,
                     cereal::NameValuePair<T> const&)
{
}

//! Epilogue for NVPs for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class Previous, class ImmerArchives, class T>
inline void epilogue(json_immer_input_archive<Previous, ImmerArchives>&,
                     cereal::NameValuePair<T> const&)
{
}

// ######################################################################
//! Prologue for deferred data for JSON archives
/*! Do nothing for the defer wrapper */
template <class Previous, class ImmerArchives, class T>
inline void prologue(json_immer_output_archive<Previous, ImmerArchives>&,
                     cereal::DeferredData<T> const&)
{
}

//! Prologue for deferred data for JSON archives
template <class Previous, class ImmerArchives, class T>
inline void prologue(json_immer_input_archive<Previous, ImmerArchives>&,
                     cereal::DeferredData<T> const&)
{
}

// ######################################################################
//! Epilogue for deferred for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class Previous, class ImmerArchives, class T>
inline void epilogue(json_immer_output_archive<Previous, ImmerArchives>&,
                     cereal::DeferredData<T> const&)
{
}

//! Epilogue for deferred for JSON archives
/*! Do nothing for the defer wrapper */
template <class Previous, class ImmerArchives, class T>
inline void epilogue(json_immer_input_archive<Previous, ImmerArchives>&,
                     cereal::DeferredData<T> const&)
{
}

// ######################################################################
//! Prologue for SizeTags for JSON archives
/*! SizeTags are strictly ignored for JSON, they just indicate
    that the current node should be made into an array */
template <class Previous, class ImmerArchives, class T>
inline void prologue(json_immer_output_archive<Previous, ImmerArchives>& ar,
                     cereal::SizeTag<T> const&)
{
    ar.makeArray();
}

//! Prologue for SizeTags for JSON archives
template <class Previous, class ImmerArchives, class T>
inline void prologue(json_immer_input_archive<Previous, ImmerArchives>&,
                     cereal::SizeTag<T> const&)
{
}

// ######################################################################
//! Epilogue for SizeTags for JSON archives
/*! SizeTags are strictly ignored for JSON */
template <class Previous, class ImmerArchives, class T>
inline void epilogue(json_immer_output_archive<Previous, ImmerArchives>&,
                     cereal::SizeTag<T> const&)
{
}

//! Epilogue for SizeTags for JSON archives
template <class Previous, class ImmerArchives, class T>
inline void epilogue(json_immer_input_archive<Previous, ImmerArchives>&,
                     cereal::SizeTag<T> const&)
{
}

// ######################################################################
//! Prologue for all other types for JSON archives (except minimal types)
/*! Starts a new node, named either automatically or by some NVP,
    that may be given data by the type about to be archived

    Minimal types do not start or finish nodes */
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<
              !std::is_arithmetic<T>::value,
              !cereal::traits::has_minimal_base_class_serialization<
                  T,
                  cereal::traits::has_minimal_output_serialization,
                  json_immer_output_archive<Previous, ImmerArchives>>::value,
              !cereal::traits::has_minimal_output_serialization<
                  T,
                  json_immer_output_archive<Previous, ImmerArchives>>::value> =
              cereal::traits::sfinae>
inline void prologue(json_immer_output_archive<Previous, ImmerArchives>& ar,
                     T const&)
{
    ar.startNode();
}

//! Prologue for all other types for JSON archives
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<
              !std::is_arithmetic<T>::value,
              !cereal::traits::has_minimal_base_class_serialization<
                  T,
                  cereal::traits::has_minimal_input_serialization,
                  json_immer_input_archive<Previous, ImmerArchives>>::value,
              !cereal::traits::has_minimal_input_serialization<
                  T,
                  json_immer_input_archive<Previous, ImmerArchives>>::value> =
              cereal::traits::sfinae>
inline void prologue(json_immer_input_archive<Previous, ImmerArchives>& ar,
                     T const&)
{
    ar.startNode();
}

// ######################################################################
//! Epilogue for all other types other for JSON archives (except minimal types)
/*! Finishes the node created in the prologue

    Minimal types do not start or finish nodes */
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<
              !std::is_arithmetic<T>::value,
              !cereal::traits::has_minimal_base_class_serialization<
                  T,
                  cereal::traits::has_minimal_output_serialization,
                  json_immer_output_archive<Previous, ImmerArchives>>::value,
              !cereal::traits::has_minimal_output_serialization<
                  T,
                  json_immer_output_archive<Previous, ImmerArchives>>::value> =
              cereal::traits::sfinae>
inline void epilogue(json_immer_output_archive<Previous, ImmerArchives>& ar,
                     T const&)
{
    ar.finishNode();
}

//! Epilogue for all other types other for JSON archives
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<
              !std::is_arithmetic<T>::value,
              !cereal::traits::has_minimal_base_class_serialization<
                  T,
                  cereal::traits::has_minimal_input_serialization,
                  json_immer_input_archive<Previous, ImmerArchives>>::value,
              !cereal::traits::has_minimal_input_serialization<
                  T,
                  json_immer_input_archive<Previous, ImmerArchives>>::value> =
              cereal::traits::sfinae>
inline void epilogue(json_immer_input_archive<Previous, ImmerArchives>& ar,
                     T const&)
{
    ar.finishNode();
}

// ######################################################################
//! Prologue for arithmetic types for JSON archives
template <class Previous, class ImmerArchives>
inline void prologue(json_immer_output_archive<Previous, ImmerArchives>& ar,
                     std::nullptr_t const&)
{
    ar.writeName();
}

//! Prologue for arithmetic types for JSON archives
template <class Previous, class ImmerArchives>
inline void prologue(json_immer_input_archive<Previous, ImmerArchives>&,
                     std::nullptr_t const&)
{
}

// ######################################################################
//! Epilogue for arithmetic types for JSON archives
template <class Previous, class ImmerArchives>
inline void epilogue(json_immer_output_archive<Previous, ImmerArchives>&,
                     std::nullptr_t const&)
{
}

//! Epilogue for arithmetic types for JSON archives
template <class Previous, class ImmerArchives>
inline void epilogue(json_immer_input_archive<Previous, ImmerArchives>&,
                     std::nullptr_t const&)
{
}

// ######################################################################
//! Prologue for arithmetic types for JSON archives
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void prologue(json_immer_output_archive<Previous, ImmerArchives>& ar,
                     T const&)
{
    ar.writeName();
}

//! Prologue for arithmetic types for JSON archives
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void prologue(json_immer_input_archive<Previous, ImmerArchives>&,
                     T const&)
{
}

// ######################################################################
//! Epilogue for arithmetic types for JSON archives
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void epilogue(json_immer_output_archive<Previous, ImmerArchives>&,
                     T const&)
{
}

//! Epilogue for arithmetic types for JSON archives
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void epilogue(json_immer_input_archive<Previous, ImmerArchives>&,
                     T const&)
{
}

// ######################################################################
//! Prologue for strings for JSON archives
template <class Previous,
          class ImmerArchives,
          class CharT,
          class Traits,
          class Alloc>
inline void prologue(json_immer_output_archive<Previous, ImmerArchives>& ar,
                     std::basic_string<CharT, Traits, Alloc> const&)
{
    ar.writeName();
}

//! Prologue for strings for JSON archives
template <class Previous,
          class ImmerArchives,
          class CharT,
          class Traits,
          class Alloc>
inline void prologue(json_immer_input_archive<Previous, ImmerArchives>&,
                     std::basic_string<CharT, Traits, Alloc> const&)
{
}

// ######################################################################
//! Epilogue for strings for JSON archives
template <class Previous,
          class ImmerArchives,
          class CharT,
          class Traits,
          class Alloc>
inline void epilogue(json_immer_output_archive<Previous, ImmerArchives>&,
                     std::basic_string<CharT, Traits, Alloc> const&)
{
}

//! Epilogue for strings for JSON archives
template <class Previous,
          class ImmerArchives,
          class CharT,
          class Traits,
          class Alloc>
inline void epilogue(json_immer_input_archive<Previous, ImmerArchives>&,
                     std::basic_string<CharT, Traits, Alloc> const&)
{
}

// ######################################################################
// Common JSONArchive serialization functions
// ######################################################################
//! Serializing NVP types to JSON
template <class Previous, class ImmerArchives, class T>
inline void CEREAL_SAVE_FUNCTION_NAME(
    json_immer_output_archive<Previous, ImmerArchives>& ar,
    cereal::NameValuePair<T> const& t)
{
    ar.setNextName(t.name);
    ar(t.value);
}

template <class Previous, class ImmerArchives, class T>
inline void
CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive<Previous, ImmerArchives>& ar,
                          cereal::NameValuePair<T>& t)
{
    ar.setNextName(t.name);
    ar(t.value);
}

//! Saving for nullptr to JSON
template <class Previous, class ImmerArchives>
inline void CEREAL_SAVE_FUNCTION_NAME(
    json_immer_output_archive<Previous, ImmerArchives>& ar,
    std::nullptr_t const& t)
{
    ar.saveValue(t);
}

//! Loading arithmetic from JSON
template <class Previous, class ImmerArchives>
inline void
CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive<Previous, ImmerArchives>& ar,
                          std::nullptr_t& t)
{
    ar.loadValue(t);
}

//! Saving for arithmetic to JSON
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void CEREAL_SAVE_FUNCTION_NAME(
    json_immer_output_archive<Previous, ImmerArchives>& ar, T const& t)
{
    ar.saveValue(t);
}

//! Loading arithmetic from JSON
template <class Previous,
          class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void
CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive<Previous, ImmerArchives>& ar,
                          T& t)
{
    ar.loadValue(t);
}

//! saving string to JSON
template <class Previous,
          class ImmerArchives,
          class CharT,
          class Traits,
          class Alloc>
inline void CEREAL_SAVE_FUNCTION_NAME(
    json_immer_output_archive<Previous, ImmerArchives>& ar,
    std::basic_string<CharT, Traits, Alloc> const& str)
{
    ar.saveValue(str);
}

//! loading string from JSON
template <class Previous,
          class ImmerArchives,
          class CharT,
          class Traits,
          class Alloc>
inline void
CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive<Previous, ImmerArchives>& ar,
                          std::basic_string<CharT, Traits, Alloc>& str)
{
    ar.loadValue(str);
}

// ######################################################################
//! Saving SizeTags to JSON
template <class Previous, class ImmerArchives, class T>
inline void
CEREAL_SAVE_FUNCTION_NAME(json_immer_output_archive<Previous, ImmerArchives>&,
                          cereal::SizeTag<T> const&)
{
    // nothing to do here, we don't explicitly save the size
}

//! Loading SizeTags from JSON
template <class Previous, class ImmerArchives, class T>
inline void
CEREAL_LOAD_FUNCTION_NAME(json_immer_input_archive<Previous, ImmerArchives>& ar,
                          cereal::SizeTag<T>& st)
{
    ar.loadSize(st.size);
}

} // namespace immer::archive

// tie input and output archives together
namespace cereal {
namespace traits {
namespace detail {
template <class Previous, class ImmerArchives>
struct get_output_from_input<
    immer::archive::json_immer_input_archive<Previous, ImmerArchives>>
{
    using type =
        immer::archive::json_immer_output_archive<Previous, ImmerArchives>;
};
template <class Previous, class ImmerArchives>
struct get_input_from_output<
    immer::archive::json_immer_output_archive<Previous, ImmerArchives>>
{
    using type =
        immer::archive::json_immer_input_archive<Previous, ImmerArchives>;
};
} // namespace detail
} // namespace traits
} // namespace cereal
