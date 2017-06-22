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

#pragma once

#include <stdexcept>
#include <cstdint>

struct no_more_input : std::exception {};

struct fuzzer_input
{
    const std::uint8_t* data_;
    std::size_t size_;

    const std::uint8_t* next(std::size_t size)
    {
        if (size_ < size) throw no_more_input{};
        auto r = data_;
        data_ += size;
        size_ -= size;
        return r;
    }

    const std::uint8_t* next(std::size_t size, std::size_t align)
    {
        auto rem = size % align;
        if (rem) next(align - rem);
        return next(size);
    }

    template <typename Fn>
    int run(Fn step)
    {
        try {
            while (step(*this));
        } catch (const no_more_input&) {};
        return 0;
    }
};

template <typename T>
const T& read(fuzzer_input& fz)
{
    return *reinterpret_cast<const T*>(fz.next(sizeof(T), alignof(T)));
}

template <typename T, typename Cond>
T read(fuzzer_input& fz, Cond cond)
{
    auto x = read<T>(fz);
    return cond(x)
        ? x
        : read<T>(fz, cond);
}
