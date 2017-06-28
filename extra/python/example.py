#!/usr/bin/env python

# immer - immutable data structures for C++
# Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
#
# This file is part of immer.
#
# immer is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# immer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with immer.  If not, see <http://www.gnu.org/licenses/>.

##

# include:intro/start
import immer

v0 = immer.Vector().append(13).append(42)
assert v0[0] == 13
assert v0[1] == 42
assert len(v0) == 2

v1 = v0.set(0, 12)
assert v0.tolist() == [13, 42]
assert v1.tolist() == [12, 42]
# include:intro/end
