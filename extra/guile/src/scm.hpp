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

#include <libguile.h>
#include <cstdint>

namespace scm {

SCM from_cpp(SCM v)           { return v; };
SCM from_cpp(float v)         { return scm_from_double(v); };
SCM from_cpp(double v)        { return scm_from_double(v); };
SCM from_cpp(std::int8_t v)   { return scm_from_int8(v); };
SCM from_cpp(std::int16_t v)  { return scm_from_int16(v); };
SCM from_cpp(std::int32_t v)  { return scm_from_int32(v); };
SCM from_cpp(std::int64_t v)  { return scm_from_int64(v); };
SCM from_cpp(std::uint8_t v)  { return scm_from_uint8(v); };
SCM from_cpp(std::uint16_t v) { return scm_from_uint16(v); };
SCM from_cpp(std::uint32_t v) { return scm_from_uint32(v); };
SCM from_cpp(std::uint64_t v) { return scm_from_uint64(v); };

template <typename T>
auto to_cpp(SCM v);
template <> auto to_cpp<SCM>           (SCM v) { return v; };
template <> auto to_cpp<float>         (SCM v) { return scm_to_double(v); };
template <> auto to_cpp<double>        (SCM v) { return scm_to_double(v); };
template <> auto to_cpp<std::int8_t>   (SCM v) { return scm_to_int8(v); };
template <> auto to_cpp<std::int16_t>  (SCM v) { return scm_to_int16(v); };
template <> auto to_cpp<std::int32_t>  (SCM v) { return scm_to_int32(v); };
template <> auto to_cpp<std::int64_t>  (SCM v) { return scm_to_int64(v); };
template <> auto to_cpp<std::uint8_t>  (SCM v) { return scm_to_uint8(v); };
template <> auto to_cpp<std::uint16_t> (SCM v) { return scm_to_uint16(v); };
template <> auto to_cpp<std::uint32_t> (SCM v) { return scm_to_uint32(v); };
template <> auto to_cpp<std::uint64_t> (SCM v) { return scm_to_uint64(v); };

} // namespace scm
