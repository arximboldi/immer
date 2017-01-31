(ns immer-benchmark
  (:require [criterium.core :as c]
            [clojure.core.rrb-vector :as fv]))

(def benchmark-size 1000)
(def benchmark-vector (vec (range benchmark-size)))
(def benchmark-rrb-vector (fv/vec (range benchmark-size)))

(defn vector-push [v]
  (loop [v v
         i 0]
    (if (< i benchmark-size)
      (recur (conj v i)
             (inc i))
      v)))

(defn vector-update [v]
  (loop [v v
         i 0]
    (if (< i benchmark-size)
      (recur (assoc v i (+ i 1))
             (inc i))
      v)))

(defn vector-iter [v]
  (reduce + 0 v))

(defn -main []
  (println "\n== VECTOR ITER ================")
  (c/with-progress-reporting (c/bench (vector-iter benchmark-vector)))
  (println "\n== VECTOR PUSH ================")
  (c/with-progress-reporting (c/bench (vector-push (vector))))
  (println "\n== VECTOR UPDATE ================")
  (c/with-progress-reporting (c/bench (vector-update benchmark-vector)))
  (println "\n== RRB-VECTOR ITER ================")
  (c/with-progress-reporting (c/bench (vector-iter benchmark-rrb-vector)))
  (println "\n== RRB-VECTOR PUSH ================")
  (c/with-progress-reporting (c/bench (vector-push (fv/vector))))
  (println "\n== RRB-VECTOR UPDATE ================")
  (c/with-progress-reporting (c/bench (vector-update benchmark-rrb-vector))))
