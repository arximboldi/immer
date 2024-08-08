#pragma once

namespace immer::persist {

/**
 * The wrapper is used to enable the incompatible_hash_mode, which is required
 * when the key of a hash-based container transformed in a way that changes its
 * hash.
 */
template <class Container>
struct incompatible_hash_wrapper
{};

/**
 * A bit of a hack but currently this is the simplest way to request a type of
 * the hash function to be used after the transformation. Maybe the whole thing
 * would change later. Right now everything is driven by the single function,
 * which seems to be convenient otherwise.
 */
struct target_container_type_request
{};

} // namespace immer::persist
