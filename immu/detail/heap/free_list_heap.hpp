
#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>

namespace immu {
namespace detail {

template <std::size_t Size, typename Base>
class free_list_heap : Base
{
    struct node_t
    {
        std::atomic<node_t*> next;
    };

public:
    using base_t = Base;

    static void* allocate(std::size_t size)
    {
        assert(size <= Size);
        node_t* n;
        do {
            n = head_.next;
            if (!n)
                return 1 + static_cast<node_t*>(
                    base_t::allocate(Size + sizeof(node_t)));
        } while (!head_.next.compare_exchange_weak(n, n->next));
        return 1 + n;
    }

    static void deallocate(void* data)
    {
        auto n = static_cast<node_t*>(data) - 1;
        do {
            n->next = head_.next;
        } while (!head_.next.compare_exchange_weak(n->next, n));
    }

private:
    static node_t head_;
};

} // namespace detail
} // namespace immu
