#pragma once

#include <immer/vector.hpp>

#include <spdlog/spdlog.h>

namespace immer_archive {

template <typename Node>
struct ptr_with_deleter
{
    Node* ptr;
    std::function<void(Node* ptr)> deleter;

    ptr_with_deleter()
        : ptr{nullptr}
        , deleter{}
    {
    }

    ptr_with_deleter(Node* ptr_, std::function<void(Node* ptr)> deleter_)
        : ptr{ptr_}
        , deleter{std::move(deleter_)}
    {
    }

    void dec() const
    {
        if (ptr && ptr->dec()) {
            SPDLOG_TRACE("calling deleter for {}", (void*) ptr);
            deleter(ptr);
        }
    }

    friend void swap(ptr_with_deleter& x, ptr_with_deleter& y)
    {
        using std::swap;
        swap(x.ptr, y.ptr);
        swap(x.deleter, y.deleter);
    }
};

template <typename Node>
class node_ptr
{
public:
    node_ptr() = default;

    node_ptr(Node* ptr_, std::function<void(Node* ptr)> deleter_)
        : ptr{ptr_, std::move(deleter_)}
    {
        SPDLOG_TRACE("ctor {} with ptr {}", (void*) this, (void*) ptr.ptr);
        // Assuming the node has just been created and not calling inc() on
        // it.
    }

    node_ptr(const node_ptr& other)
        : ptr{other.ptr}
    {
        SPDLOG_TRACE("copy ctor {} from {}", (void*) this, (void*) &other);
        if (ptr.ptr) {
            ptr.ptr->inc();
        }
    }

    node_ptr(node_ptr&& other)
        : node_ptr{}
    {
        SPDLOG_TRACE("move ctor {} from {}", (void*) this, (void*) &other);
        swap(*this, other);
    }

    node_ptr& operator=(const node_ptr& other)
    {
        SPDLOG_TRACE("copy assign {} = {}", (void*) this, (void*) &other);
        auto temp = other;
        swap(*this, temp);
        return *this;
    }

    node_ptr& operator=(node_ptr&& other)
    {
        SPDLOG_TRACE("move assign {} = {}", (void*) this, (void*) &other);
        auto temp = node_ptr{std::move(other)};
        swap(*this, temp);
        return *this;
    }

    ~node_ptr()
    {
        SPDLOG_TRACE("dtor {}", (void*) this);
        ptr.dec();
    }

    explicit operator bool() const { return ptr.ptr; }

    Node* release() &&
    {
        auto result = ptr.ptr;
        ptr.ptr     = nullptr;
        return result;
    }

    auto release_full() &&
    {
        auto result = ptr;
        ptr         = {};
        return result;
    }

    Node* get() { return ptr.ptr; }

    friend void swap(node_ptr& x, node_ptr& y)
    {
        using std::swap;
        swap(x.ptr, y.ptr);
    }

private:
    ptr_with_deleter<Node> ptr;
};

} // namespace immer_archive
