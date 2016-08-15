
#pragma once

#include <cstdlib>
#include <cassert>

namespace immu {
namespace detail {

template <std::size_t Size, typename Base>
class thread_local_free_list_heap : Base
{
    struct node_t
    {
        node_t* next;
    };

public:
    using base_t = Base;

    static void* allocate(std::size_t size)
    {
        assert(size <= Size);
        node_t* n;
        n = head_.next;
        if (!n)
            return 1 + static_cast<node_t*>(
                base_t::allocate(Size + sizeof(node_t)));
        head_.next = n->next;
        return 1 + n;
    }

    static void deallocate(void* data)
    {
        auto n = static_cast<node_t*>(data) - 1;
        n->next = head_.next;
        head_.next = n;
    }

private:
    thread_local static node_t head_;
};

template <std::size_t S, typename B>
thread_local typename thread_local_free_list_heap<S, B>::node_t
thread_local_free_list_heap<S, B>::head_ {nullptr};

} // namespace detail
} // namespace immu
