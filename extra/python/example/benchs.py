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

import benchmark

import pyrsistent
import immer

BENCHMARK_SIZE = 1000
BENCHMARK_RUNS = 20

def tov(v, src=xrange(BENCHMARK_SIZE)):
    for x in src:
        v = v.append(x)
    return v

IMMER_EMPTY_VECTOR = immer.Vector()
PYRSISTENT_EMPTY_VECTOR = pyrsistent.pvector()

IMMER_FULL_VECTOR = tov(immer.Vector())
PYRSISTENT_FULL_VECTOR = tov(pyrsistent.pvector())

class Benchmark_Push(benchmark.Benchmark):
    each = BENCHMARK_RUNS

    def test_immer(self):
        v = immer.Vector()
        for i in xrange(BENCHMARK_SIZE):
            v = v.append(i)

    def test_pyrsistent(self):
        v = pyrsistent.pvector()
        for i in xrange(BENCHMARK_SIZE):
            v = v.append(i)

class Benchmark_Set(benchmark.Benchmark):
    each = BENCHMARK_RUNS

    def test_immer(self):
        v = IMMER_FULL_VECTOR
        for i in xrange(BENCHMARK_SIZE):
            v = v.set(i, i+1)

    def test_pyrsistent(self):
        v = PYRSISTENT_FULL_VECTOR
        for i in xrange(BENCHMARK_SIZE):
            v = v.set(i, i+1)

class Benchmark_Access(benchmark.Benchmark):
    each = BENCHMARK_RUNS

    def test_immer(self):
        v = IMMER_FULL_VECTOR
        for i in xrange(BENCHMARK_SIZE):
            v[i]

    def test_pyrsistent(self):
        v = PYRSISTENT_FULL_VECTOR
        for i in xrange(BENCHMARK_SIZE):
            v[i]

if hasattr(IMMER_FULL_VECTOR, 'tolist'):
    class Benchmark_ToList(benchmark.Benchmark):
        each = BENCHMARK_RUNS

        def test_immer(self):
            IMMER_FULL_VECTOR.tolist()

        def test_pyrsistent(self):
            PYRSISTENT_FULL_VECTOR.tolist()

class Benchmark_Size(benchmark.Benchmark):
    each = BENCHMARK_RUNS

    def test_immer(self):
        for i in xrange(BENCHMARK_SIZE):
            len(IMMER_EMPTY_VECTOR)

    def test_pyrsistent(self):
        for i in xrange(BENCHMARK_SIZE):
            len(PYRSISTENT_EMPTY_VECTOR)

    def test_immer_fn(self):
        fn = IMMER_EMPTY_VECTOR.__len__
        for i in xrange(BENCHMARK_SIZE):
            fn()

    def test_pyrsistent_fn(self):
        fn = PYRSISTENT_EMPTY_VECTOR.__len__
        for i in xrange(BENCHMARK_SIZE):
            fn()

if __name__ == '__main__':
    benchmark.main(format="markdown",
                   numberFormat="%.4g")
