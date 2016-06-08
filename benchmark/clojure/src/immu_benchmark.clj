(ns immu-benchmark
  (:require [criterium.core :as c]))

(def benchmark-size 1000)

(def benchmark-vector (vec (range benchmark-size)))

(defn vektor-push []
  (loop [v (vector)
         i 0]
    (if (< i benchmark-size)
      (recur (conj v i)
             (inc i))
      v)))

(defn vektor-iter []
  (reduce + 0 benchmark-vector))

(defn -main []
  (c/with-progress-reporting (c/bench (vektor-push)))
  (c/with-progress-reporting (c/bench (vektor-iter))))
