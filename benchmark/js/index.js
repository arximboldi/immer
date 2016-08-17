
var immu = Module

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
    .add('immu.Vektor', function(){
        var v = new immu.Vektor
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
    })
    .add('immu.VektorInt', function(){
        var v = new immu.VektorInt
        for (var x = 0; x < N; ++x) {
            var v_ = v
            v = v.push(x)
            v_.delete()
        }
    })
    .add('immu.VektorNumber', function(){
        var v = new immu.VektorNumber
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
