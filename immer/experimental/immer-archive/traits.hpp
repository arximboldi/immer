#pragma once

namespace immer_archive {

/**
 * Define these traits to connect a type (vector_one<T>) to its archive
 * (archive_save<T>).
 */
template <class T>
struct container_traits
{};

} // namespace immer_archive
