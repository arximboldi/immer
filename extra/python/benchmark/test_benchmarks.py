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
import pyrsistent

BENCHMARK_SIZE = 1000

def push(v, n=BENCHMARK_SIZE):
    for x in xrange(n):
        v = v.append(x)
    return v

def assoc(v):
    for i in xrange(len(v)):
        v = v.set(i, i+1)
    return v

def index(v):
    for i in xrange(len(v)):
        v[i]

def test_push_immer(benchmark):
    benchmark(push, immer.Vector())

def test_push_pyrsistent(benchmark):
    benchmark(push, pyrsistent.pvector())

def test_assoc_immer(benchmark):
    benchmark(assoc, push(immer.Vector()))

def test_assoc_pyrsistent(benchmark):
    benchmark(assoc, push(pyrsistent.pvector()))

def test_index_immer(benchmark):
    benchmark(index, push(immer.Vector()))

def test_index_pyrsistent(benchmark):
    benchmark(index, push(pyrsistent.pvector()))
