
#pragma once

#include <immu/heap/free_list_node.hpp>
#include <cassert>

namespace immu {

template <std::size_t Size, typename Base>
class thread_local_free_list_heap : Base
{
    struct head_t
    {
        free_list_node* data;
        ~head_t() { clear(); }
    };

public:
    using base_t = Base;

    static void* allocate(std::size_t size)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        free_list_node* n;
        n = head_.data;
        if (!n) {
            auto p = base_t::allocate(Size + sizeof(free_list_node));
            return static_cast<free_list_node*>(p);
        }
        head_.data = n->next;
        return n;
    }

    static void deallocate(void* data)
    {
        auto n = static_cast<free_list_node*>(data);
        n->next = head_.data;
        head_.data = n;
    }

    static void clear()
    {
        while (head_.data) {
            auto n = head_.data->next;
            base_t::deallocate(head_.data);
            head_.data = n;
        }
    }

private:
    thread_local static head_t head_;
};

template <std::size_t S, typename B>
thread_local typename thread_local_free_list_heap<S, B>::head_t
thread_local_free_list_heap<S, B>::head_ {{nullptr}};

} // namespace immu
