#pragma once

#include <string>

namespace immer_archive {

template <typename T>
struct xx_hash;

std::size_t xx_hash_value_string(const std::string& str);

template <class T, class U>
using enable_for = std::enable_if_t<std::is_same_v<T, U>, std::size_t>;

template <class T>
enable_for<T, std::string> xx_hash_value(const T& str)
{
    return xx_hash_value_string(str);
}

template <class T>
struct xx_hash
{
    std::size_t operator()(const T& val) const { return xx_hash_value(val); }
};

} // namespace immer_archive
