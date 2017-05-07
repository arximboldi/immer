//
// immer - immutable data structures for C++
// Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
//
// This file is part of immer.
//
// immer is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// immer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with immer.  If not, see <http://www.gnu.org/licenses/>.
//

#include <immer/detail/util.hpp>
#include <immer/detail/combine_standard_layout.hpp>

#include <limits>

namespace immer {
namespace detail {

template <typename T, typename MemoryPolicy>
struct array_impl
{
    using memory      = MemoryPolicy;
    using heap        = typename memory::heap::type;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;
    using size_t      = std::size_t;

    struct holder_data_t
    {
        aligned_storage_for<T> buffer;
    };

    using holder_t = combine_standard_layout_t<holder_data_t,
                                               refs_t,
                                               ownee_t>;

    holder_t* ptr;
    size_t    size;
    size_t    capacity;

    static const array_impl empty;

    constexpr static std::size_t sizeof_holder_n(size_t count)
    {
        return offsetof(holder_t, d.buffer) + sizeof(T) * count;
    }

    array_impl(holder_t* p, size_t s, size_t c)
        : ptr{p}, size{s}, capacity{c}
    {}

    array_impl(const array_impl& other)
        : array_impl{other.ptr, other.size, other.capacity}
    {
        inc();
    }

    array_impl(array_impl&& other)
        : array_impl{empty}
    {
        swap(*this, other);
    }

    array_impl& operator=(const array_impl& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    array_impl& operator=(array_impl&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(array_impl& x, array_impl& y)
    {
        using std::swap;
        swap(x.ptr, y.ptr);
        swap(x.size, y.size);
        swap(x.capacity, y.capacity);
    }

    ~array_impl()
    {
        dec();
    }

    void inc()
    {
        using immer::detail::get;
        get<refs_t>(*ptr).inc();
    }

    void dec()
    {
        using immer::detail::get;
        if (get<refs_t>(*ptr).dec())
            delete_holder(ptr, size, capacity);
    }

    static T* data(holder_t* p) { return reinterpret_cast<T*>(&p->d.buffer); }
    const T* data() const { return data(ptr); }
    T* data()             { return data(ptr); }

    static holder_t* make_holder_n(size_t n)
    {
        return new (heap::allocate(sizeof_holder_n(n))) holder_t{};
    }

    template <typename Iter>
    static holder_t* copy_holder_n(size_t n, Iter first, Iter last)
    {
        auto p = make_holder_n(n);
        try {
            std::uninitialized_copy(first, last, data(p));
            return p;
        } catch (...) {
            heap::deallocate(sizeof_holder_n(n), p);
            throw;
        }
    }

    static holder_t* copy_holder_n(size_t n, holder_t* p, size_t count)
    {
        return copy_holder_n(n, data(p), data(p) + count);
    }

    static void delete_holder(holder_t* p, size_t sz, size_t cap)
    {
        destroy_n(data(p), sz);
        heap::deallocate(sizeof_holder_n(cap), p);
    }

    template <typename Iter>
    static array_impl from_range(Iter first, Iter last)
    {
        auto count = static_cast<size_t>(std::distance(first, last));
        return {
            copy_holder_n(count, first, last),
            count,
            count
        };
    }

    template <typename U>
    static array_impl from_initializer_list(std::initializer_list<U> values)
    {
        using namespace std;
        return from_range(begin(values), end(values));
    }

    const T& get(std::size_t index) const
    {
        return data()[index];
    }

    bool equals(const array_impl& other) const
    {
        return ptr == other.ptr ||
            std::equal(data(), data() + size, other.data());
    }

    static size_t recommend(size_t sz, size_t cap)
    {
        auto max = std::numeric_limits<size_t>::max();
        return
            sz <= cap       ? cap :
            cap >= max / 2  ? max
            /* otherwise */ : std::max(2 * cap, sz);
    }

    array_impl push_back(T value) const
    {
        auto cap = recommend(size + 1, capacity);
        auto p = copy_holder_n(cap, ptr, size);
        try {
            new (data(p) + size) T{std::move(value)};
            return { p, size + 1, cap };
        } catch (...) {
            delete_holder(p, size, cap);
            throw;
        }
    }

    array_impl assoc(std::size_t idx, T value) const
    {
        auto p = copy_holder_n(capacity, ptr, size);
        try {
            data(p)[idx] = std::move(value);
            return { p, size, capacity };
        } catch (...) {
            delete_holder(p, size, capacity);
            throw;
        }
    }

    template <typename Fn>
    array_impl update(std::size_t idx, Fn&& op) const
    {
        auto p = copy_holder_n(capacity, ptr, size);
        try {
            auto& elem = data(p)[idx];
            elem = std::forward<Fn>(op)(std::move(elem));
            return { p, size, capacity };
        } catch (...) {
            delete_holder(p, size, capacity);
            throw;
        }
    }
};

template <typename T, typename MP>
const array_impl<T, MP> array_impl<T, MP>::empty = {
    make_holder_n(1),
    0,
    1,
};

} // namespace detail
} // namespace immer
