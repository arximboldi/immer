
#pragma once

#include <atomic>
#include <cstdlib>
#include <cassert>

namespace immu {
namespace detail {

template <std::size_t Size>
class with_free_list
{
    struct node_t
    {
        std::atomic<node_t*> next;
    };

public:
    static void* operator new (std::size_t size)
    {
        assert(size <= Size);
        node_t* n;
        do {
            n = head_.next;
            if (!n)
                return 1 + static_cast<node_t*>(
                    std::malloc(Size + sizeof(node_t)));
        } while (!head_.next.compare_exchange_weak(n, n->next));
        return 1 + n;
    }

    static void operator delete (void* data)
    {
        auto n = static_cast<node_t*>(data) - 1;
        do {
            n->next = head_.next;
        } while (!head_.next.compare_exchange_weak(n->next, n));
    }

private:
    static node_t head_;
};

template <std::size_t S>
typename with_free_list<S>::node_t with_free_list<S>::head_ {nullptr};

} /* namespace detail */
} /* namespace immu */
