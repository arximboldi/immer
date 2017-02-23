(ns immer-benchmark
  (:require [criterium.core :as c]
            [clojure.core.rrb-vector :as fv]))

(def benchmark-size 100000)
(def benchmark-samples 20)

(defn vector-push-f [v]
  (loop [v v
         i 0]
    (if (< i benchmark-size)
      (recur (fv/catvec (fv/vector i) v)
             (inc i))
      v)))

(defn vector-push [v]
  (loop [v v
         i 0]
    (if (< i benchmark-size)
      (recur (conj v i)
             (inc i))
      v)))

(defn vector-concat [v vc]
  (loop [v v
         i 0]
    (if (< i benchmark-size)
      (recur (fv/catvec v vc)
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

(def benchmark-vector (vec (range benchmark-size)))
(def benchmark-rrb-vector (fv/vec (range benchmark-size)))
(def benchmark-rrb-vector-short (fv/vec (range (/ benchmark-size 2))))
(def benchmark-rrb-vector-f (vector-push-f (fv/vector)))

(defn -main []
  (println "\n== VECTOR ITER ================")
  (c/with-progress-reporting (c/bench (vector-iter benchmark-vector)
                                      :samples benchmark-samples))
  (println "\n== VECTOR PUSH ================")
  (c/with-progress-reporting (c/bench (vector-push (vector))
                                      :samples benchmark-samples))
  (println "\n== VECTOR UPDATE ================")
  (c/with-progress-reporting (c/bench (vector-update benchmark-vector)
                                      :samples benchmark-samples))
  (println "\n== RRB-VECTOR ITER ================")
  (c/with-progress-reporting (c/bench (vector-iter benchmark-rrb-vector)
                                      :samples benchmark-samples))
  (println "\n== RRB-VECTOR PUSH ================")
  (c/with-progress-reporting (c/bench (vector-push (fv/vector))
                                      :samples benchmark-samples))
  (println "\n== RRB-VECTOR UPDATE ================")
  (c/with-progress-reporting (c/bench (vector-update benchmark-rrb-vector)
                                      :samples benchmark-samples))
  (println "\n== RRB-VECTOR CONCAT ================")
  (c/with-progress-reporting (c/bench (vector-concat (fv/vector)
                                                     benchmark-rrb-vector-short)
                                      :samples benchmark-samples))
  (println "\n== RRB-VECTOR/F ITER ================")
  (c/with-progress-reporting (c/bench (vector-iter benchmark-rrb-vector-f)
                                      :samples benchmark-samples))
  (println "\n== RRB-VECTOR/F UPDATE ================")
  (c/with-progress-reporting (c/bench (vector-update benchmark-rrb-vector-f)
                                      :samples benchmark-samples)))
