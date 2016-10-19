
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
            v = mori.conj(v, x)
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
    })
    .add('immer.VectorNumber', function(){
        var v = new immer.VectorNumber
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
    })
    .on('cycle', function(event) {
        console.log(String(event.target));
    })
    .on('complete', function() {
        console.log('Fastest is ' + this.filter('fastest').map('name'));
    })
    .run({ 'async': true })
