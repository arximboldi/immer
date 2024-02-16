#include "xxhash.hpp"

#include <xxhash.h>

namespace immer_archive {

static_assert(sizeof(std::size_t) == 8); // 64 bits
static_assert(sizeof(XXH64_hash_t) == sizeof(std::size_t));

std::size_t xx_hash_value_string(const std::string& str)
{
    return XXH3_64bits(str.c_str(), str.size());
}

} // namespace immer_archive
