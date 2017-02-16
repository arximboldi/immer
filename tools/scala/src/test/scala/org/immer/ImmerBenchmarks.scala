package org.immer

import org.scalameter.api._
import scala.collection.immutable.Vector
import scala.collection.immutable.rrbvector.RRBVector

object ImmerBenchmarks extends Bench.OfflineRegressionReport {

  val sizes = Gen.range("size")(1000, 1000, 1)

  var vectors    = for { s <- sizes } yield Vector.empty[Int] ++ (0 until s)
  var rrbvectors = for { s <- sizes } yield RRBVector.empty[Int] ++ (0 until s)

  performance of "push" in {
    measure method "Vector" in { using(sizes) in { s => {
      var v = Vector.empty[Int]
      for (a <- 0 to s) {
        v = v :+ a
      }
      v
    }}}

    measure method "RRBVector" in { using(sizes) in { s => {
      var v = RRBVector.empty[Int]
      for (a <- 0 to s) {
        v = v :+ a
      }
      v
    }}}
  }

  performance of "update" in {
    measure method "Vector" in { using(vectors) in { v0 => {
      var v = v0
      for (a <- 0 to v.size - 1) {
        v = v.updated(a, a + 1)
      }
      v
    }}}

    measure method "Vector-Random" in { using(vectors) in { v0 => {
      var v = v0
      var r = new scala.util.Random
      for (a <- 0 to v.size - 1) {
        v = v.updated(r.nextInt(v.size), a + 1)
      }
      v
    }}}

    measure method "RRBVector" in { using(rrbvectors) in { v0 => {
      var v = v0
      for (a <- 0 to v.size - 1) {
        v = v.updated(a, a + 1)
      }
      v
    }}}

    measure method "RRBVector-Random" in { using(rrbvectors) in { v0 => {
      var v = v0
      var r = new scala.util.Random
      for (a <- 0 to v.size - 1) {
        v = v.updated(r.nextInt(v.size), a + 1)
      }
      v
    }}}
  }

  performance of "iter" in {
    measure method "Vector" in { using(vectors) in { v => {
      v.reduce(_ + _)
    }}}

    measure method "RRBVector" in { using(rrbvectors) in { v => {
      v.reduce(_ + _)
    }}}
  }

  performance of "concat" in {
    measure method "RRBVector ++" in { using(rrbvectors) in { v => {
      v ++ v
    }}}

    measure method "RRBVector :+" in { using(rrbvectors) in { v => {
      v :+ v
    }}}
  }
}
