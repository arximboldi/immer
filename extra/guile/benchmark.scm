;; immer - immutable data structures for C++
;; Copyright (C) 2016, 2017 Juan Pedro Bolivar Puente
;;
;; This file is part of immer.
;;
;; immer is free software: you can redistribute it and/or modify
;; it under the terms of the GNU Lesser General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; immer is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU Lesser General Public License for more details.
;;
;; You should have received a copy of the GNU Lesser General Public License
;; along with immer.  If not, see <http://www.gnu.org/licenses/>.

(use-modules (immer)
             (srfi srfi-1)
             (srfi srfi-43)
             (ice-9 vlist)
             (ice-9 pretty-print))

(define-syntax display-eval
  (syntax-rules ()
    ((_ expr)
     (begin (pretty-print 'expr
                          #:max-expr-width 72)
            expr))))

(display-eval (define bench-size 1000000))
(display-eval (define bench-samples 10))

(define (average . ns)
  (/ (apply + ns) (length ns)))

(define (generate-n n fn)
  (unfold (lambda (x) (= x n))
          (lambda (x) (fn))
          (lambda (x) (+ x 1))
          0))

(define-syntax benchmark
  (syntax-rules ()
    ((_ expr)
     (begin
       (display "; evaluating:    ") (newline)
       (pretty-print 'expr
                     #:max-expr-width 72
                     #:per-line-prefix "      ")
       (let* ((sample (lambda ()
                        (gc)
                        (let* ((t0 (get-internal-real-time))
                               (r  expr)
                               (t1 (get-internal-real-time)))
                          (/ (- t1 t0) internal-time-units-per-second))))
              (samples (generate-n bench-samples sample))
              (result (apply average samples)))
         (display "; average time: ")
         (display (exact->inexact result))
         (display " seconds")
         (newline))))))

(display ";;;; benchmarking creation...") (newline)

(benchmark (apply ivector (iota bench-size)))
(benchmark (apply ivector-u32 (iota bench-size)))
(benchmark (iota bench-size))
(benchmark (apply vector (iota bench-size)))
(benchmark (apply u32vector (iota bench-size)))
(benchmark (list->vlist (iota bench-size)))

(display ";;;; benchmarking iteration...") (newline)

(display-eval (define bench-ivector (apply ivector (iota bench-size))))
(display-eval (define bench-ivector-u32 (apply ivector-u32 (iota bench-size))))
(display-eval (define bench-list (iota bench-size)))
(display-eval (define bench-vector (apply vector (iota bench-size))))
(display-eval (define bench-u32vector (apply u32vector (iota bench-size))))
(display-eval (define bench-vlist (list->vlist (iota bench-size))))

(benchmark (ivector-fold + 0 bench-ivector))
(benchmark (ivector-u32-fold + 0 bench-ivector-u32))
(benchmark (fold + 0 bench-list))
(benchmark (vector-fold + 0 bench-vector))
(benchmark (vlist-fold + 0 bench-vlist))

(display ";;;; benchmarking iteration by index...") (newline)
(benchmark (let iter ((i 0) (acc 0))
             (if (< i (ivector-length bench-ivector))
                 (iter (+ i 1)
                       (+ acc (ivector-ref bench-ivector i)))
                 acc)))
(benchmark (let iter ((i 0) (acc 0))
             (if (< i (ivector-u32-length bench-ivector-u32))
                 (iter (+ i 1)
                       (+ acc (ivector-u32-ref bench-ivector-u32 i)))
                 acc)))
(benchmark (let iter ((i 0) (acc 0))
             (if (< i (vector-length bench-vector))
                 (iter (+ i 1)
                       (+ acc (vector-ref bench-vector i)))
                 acc)))
(benchmark (let iter ((i 0) (acc 0))
             (if (< i (u32vector-length bench-u32vector))
                 (iter (+ i 1)
                       (+ acc (u32vector-ref bench-u32vector i)))
                 acc)))
(benchmark (let iter ((i 0) (acc 0))
             (if (< i (vlist-length bench-vlist))
                 (iter (+ i 1)
                       (+ acc (vlist-ref bench-vlist i)))
                 acc)))

(display ";;;; benchmarking concatenation...") (newline)
(benchmark (ivector-append bench-ivector bench-ivector))
(benchmark (ivector-u32-append bench-ivector-u32 bench-ivector-u32))
(benchmark (append bench-list bench-list))
(benchmark (vector-append bench-vector bench-vector))
(benchmark (vlist-append bench-vlist bench-vlist))
