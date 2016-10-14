
#pragma once

#include <immer/heap/with_data.hpp>
#include <immer/heap/free_list_node.hpp>

#include <atomic>
#include <cassert>

namespace immer {

template <std::size_t Size, typename Base>
struct free_list_heap : Base
{
    using base_t = Base;

    template <typename Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        free_list_node* n;
        do {
            n = head_;
            if (!n) {
                auto p = base_t::allocate(Size + sizeof(free_list_node));
                return static_cast<free_list_node*>(p);
            }
        } while (!head_.compare_exchange_weak(n, n->next));
        return n;
    }

    template <typename Tags>
    static void deallocate(void* data, Tags...)
    {
        auto n = static_cast<free_list_node*>(data);
        do {
            n->next = head_;
        } while (!head_.compare_exchange_weak(n->next, n));
    }

private:
    using head_t = std::atomic<free_list_node*>;
    static head_t head_;
};

template <std::size_t S, typename B>
typename free_list_heap<S, B>::head_t
free_list_heap<S, B>::head_ {nullptr};

} // namespace immer
