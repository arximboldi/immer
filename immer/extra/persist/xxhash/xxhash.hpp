#pragma once

#include <string>

namespace immer::persist {

template <class T>
struct xx_hash
{
    std::size_t operator()(const T& val) const { return xx_hash_value(val); }
};

std::size_t xx_hash_value_string(const std::string& str);

template <>
struct xx_hash<std::string>
{
    std::size_t operator()(const std::string& val) const
    {
        return xx_hash_value_string(val);
    }
};

} // namespace immer::persist
