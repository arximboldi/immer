
#pragma once

#include <immer/heap/free_list_node.hpp>
#include <cassert>

namespace immer {
namespace detail {

template <typename Heap>
struct unsafe_free_list_storage
{
    struct head_t
    {
        free_list_node* data;
    };

    static head_t head;
};

template <typename Heap>
typename unsafe_free_list_storage<Heap>::head_t
unsafe_free_list_storage<Heap>::head {{nullptr}};

template <typename Heap>
struct thread_local_free_list_storage
{
    struct head_t
    {
        free_list_node* data;
        ~head_t() { Heap::clear(); }
    };

    thread_local static head_t head;
};

template <typename Heap>
thread_local typename thread_local_free_list_storage<Heap>::head_t
thread_local_free_list_storage<Heap>::head {nullptr};

template <template<class>class Storage,
          std::size_t Size,
          typename Base>
class unsafe_free_list_heap_impl : public Base
{
    using storage = Storage<unsafe_free_list_heap_impl>;

public:
    using base_t = Base;

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        free_list_node* n;
        n = storage::head.data;
        if (!n) {
            auto p = base_t::allocate(Size + sizeof(free_list_node));
            return static_cast<free_list_node*>(p);
        }
        storage::head.data = n->next;
        return n;
    }

    template <typename... Tags>
    static void deallocate(void* data, Tags...)
    {
        auto n = static_cast<free_list_node*>(data);
        n->next = storage::head.data;
        storage::head.data = n;
    }

    static void clear()
    {
        while (storage::head.data) {
            auto n = storage::head.data->next;
            base_t::deallocate(storage::head.data);
            storage::head.data = n;
        }
    }
};

} // namespace detail

template <std::size_t Size, typename Base>
struct thread_local_free_list_heap : detail::unsafe_free_list_heap_impl<
    detail::thread_local_free_list_storage,
    Size,
    Base>
{};

template <std::size_t Size, typename Base>
struct unsafe_free_list_heap : detail::unsafe_free_list_heap_impl<
    detail::unsafe_free_list_storage,
    Size,
    Base>
{};

} // namespace immer
