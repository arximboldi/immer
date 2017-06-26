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

#include <scm/types.hpp>
#include <iostream>

namespace scm {

struct list
{
    list() = default;
    list(SCM value) : handle_{value} {}

    operator SCM () const { return handle_; }

    list end() const { return {}; }
    list begin() const { return *this; }

    explicit operator bool() { return handle_ != SCM_EOL; }

    SCM operator* () const { return scm_car(handle_); }

    list& operator++ ()
    {
        handle_ = scm_cdr(handle_);
        return *this;
    }

    list operator++ (int)
    {
        auto result = *this;
        result.handle_ = scm_cdr(handle_);
        return result;
    }

    bool operator==(list other) { return handle_ == other.handle_; }
    bool operator!=(list other) { return handle_ != other.handle_; }

private:
    SCM handle_ = SCM_EOL;
};

struct args : list
{
    using list::list;
};

} // namespace scm

SCM_DECLARE_WRAPPER_TYPE(scm::list);
SCM_DECLARE_WRAPPER_TYPE(scm::args);
