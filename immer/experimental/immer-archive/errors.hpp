#pragma once

#include <immer/experimental/immer-archive/common/archive.hpp>

#include <stdexcept>

#include <fmt/format.h>

namespace immer_archive {

class archive_exception : public std::invalid_argument
{
public:
    using invalid_argument::invalid_argument;
};

class archive_has_cycles : public archive_exception
{
public:
    explicit archive_has_cycles(node_id id)
        : archive_exception{fmt::format("Cycle detected with node ID {}", id)}
    {
    }
};

class invalid_node_id : public archive_exception
{
public:
    explicit invalid_node_id(node_id id)
        : archive_exception{fmt::format("Node ID {} is not found", id)}
    {
    }
};

class invalid_container_id : public archive_exception
{
public:
    explicit invalid_container_id(container_id id)
        : archive_exception{fmt::format("Container ID {} is not found", id)}
    {
    }
};

class invalid_children_count : public archive_exception
{
public:
    explicit invalid_children_count(node_id id)
        : archive_exception{
              fmt::format("Node ID {} has more children than allowed", id)}
    {
    }
};

} // namespace immer_archive
