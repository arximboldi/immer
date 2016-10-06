
#pragma once

#include <immer/heap/with_data.hpp>

namespace immer {

struct free_list_node
{
    free_list_node* next;
};

template <typename Base>
struct with_free_list_node
    : with_data<free_list_node, Base>
{};

} // namespace immer
