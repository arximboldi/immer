
#pragma once

#include <immu/detail/heap/with_data.hpp>

namespace immu {
namespace detail {

struct free_list_node
{
    free_list_node* next;
};

template <typename Base>
struct with_free_list_node
    : with_data<free_list_node, Base>
{};

} // namespace detail
} // namespace immu
