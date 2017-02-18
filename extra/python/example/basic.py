#!/usr/bin/env python
#
# immer - immutable data structures for C++
# Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
#
# This file is part of immer.
#
# immer is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# immer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with immer.  If not, see <http://www.gnu.org/licenses/>.
#

import immer

v0 = immer.Vector().append(13).append(42)
assert v0[0] == 13 and v0[1] == 42 and len(v0) == 2

v1 = v0.set(0, 12)
assert v0[0] == 13 and v0[1] == 42 and len(v0) == 2
assert v1[0] == 12 and v1[1] == 42 and len(v1) == 2
