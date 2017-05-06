/*
 * immer - immutable data structures for C++
 * Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
 *
 * This file is part of immer.
 *
 * immer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * immer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with immer.  If not, see <http://www.gnu.org/licenses/>.
 */

var immer = Module

var N = 1000

var suite = new Benchmark.Suite('push')
    .add('Immutable.List', function(){
        var v = new Immutable.List
        for (var x = 0; x < N; ++x)
            v = v.push(x)
    })
    .add('mori.vector', function(){
        var v = mori.vector()
        for (var x = 0; x < N; ++x)
            v = mori.conj.f2(v, x)
    })
    .add('mori.vector-Transient', function(){
        var v = mori.mutable.thaw(mori.vector())
        for (var x = 0; x < N; ++x)
            v = mori.mutable.conj.f2(v, x)
        return mori.mutable.freeze(v)
    })
    .add('immer.Vector', function(){
        var v = new immer.Vector
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
    })
    .add('immer.VectorInt', function(){
        var v = new immer.VectorInt
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
        v.delete()
    })
    .add('immer.VectorNumber', function(){
        var v = new immer.VectorNumber
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
        v.delete()
    })
    .add('immer.VectorInt-Native', function(){
        immer.rangeSlow_int(0, N).delete()
    })
    .add('immer.VectorInt-NativeTransient', function(){
        immer.range_int(0, N).delete()
    })
    .add('immer.VectorDouble-Native', function(){
        immer.rangeSlow_double(0, N).delete()
    })
    .add('immer.VectorDouble-NativeTransient', function(){
        immer.range_double(0, N).delete()
    })
    .on('cycle', function(event) {
        console.log(String(event.target));
    })
    .on('complete', function() {
        console.log('Fastest is ' + this.filter('fastest').map('name'));
    })
    .run({ 'async': true })
