#pragma once

#include <immer/extra/persist/common/pool.hpp>

#include <stdexcept>

#include <fmt/format.h>

namespace immer::persist {

class pool_exception : public std::invalid_argument
{
public:
    using invalid_argument::invalid_argument;
};

class pool_has_cycles : public pool_exception
{
public:
    explicit pool_has_cycles(node_id id)
        : pool_exception{fmt::format("Cycle detected with node ID {}", id)}
    {
    }
};

class invalid_node_id : public pool_exception
{
public:
    explicit invalid_node_id(node_id id)
        : pool_exception{fmt::format("Node ID {} is not found", id)}
    {
    }
};

class invalid_container_id : public pool_exception
{
public:
    explicit invalid_container_id(container_id id)
        : pool_exception{fmt::format("Container ID {} is not found", id)}
    {
    }
};

class invalid_children_count : public pool_exception
{
public:
    explicit invalid_children_count(node_id id)
        : pool_exception{
              fmt::format("Node ID {} has more children than allowed", id)}
    {
    }
};

} // namespace immer::persist
