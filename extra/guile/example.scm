(use-modules (immer)
             (srfi srfi-1)
             (srfi srfi-43)
             (ice-9 vlist)
             (rnrs base))

;;
;; Experiments
;;

(let ((d (dummy)))
  (dummy-foo d))
(gc)

(foo)
(bar)
(foo-bar)

;;
;; Showcase immer API
;;

(let ((v1 (ivector 1 "hola" 3 'que #:tal)))
  (assert (eq? (ivector-ref v1 3) 'que))

  (let* ((v2 (ivector-set v1 3 'what))
         (v2 (ivector-update v2 2 (lambda (x) (+ 1 x)))))
    (assert (eq? (ivector-ref v1 2) 3))
    (assert (eq? (ivector-ref v1 3) 'que))
    (assert (eq? (ivector-ref v2 2) 4))
    (assert (eq? (ivector-ref v2 3) 'what))

    (let ((v3 (ivector-push v2 "hehe")))
      (assert (eq? (ivector-length v3) 6))
      (assert (eq? (ivector-ref v3 (- (ivector-length v3) 1)) "hehe")))))

(let ((v (apply ivector (iota 10))))
  (assert (eq? (ivector-length v) 10))
  (assert (eq? (ivector-length (ivector-drop v 3)) 7))
  (assert (eq? (ivector-length (ivector-take v 3)) 3)))

(let ((v1 (make-ivector 3))
      (v2 (make-ivector 3 ":)")))
  (assert (eq? (ivector-ref v1 2)
               (vector-ref (make-vector 3) 2)))
  (assert (eq? (ivector-ref v2 2) ":)")))

;;
;; Some micro benchmakrs
;;

(define-syntax mini-bench
  (syntax-rules ()
    ((_ expr)
     (begin
       (display "-- evaluating:    ")
       (display 'expr)
       (newline)
       (let* ((t0 (get-internal-real-time))
              (r  expr)
              (t1 (get-internal-real-time))
              (dt (/ (- t1 t0) internal-time-units-per-second)))
         (display "   evaluated in:  ")
         (display (exact->inexact dt))
         (display " s")
         (newline))))))

(define bench-size 100000)

(newline)
(display "== benchmarking creation...") (newline)
(mini-bench (apply ivector (iota bench-size)))
;;(mini-bench (apply ivector-u32 (iota bench-size)))
(mini-bench (iota bench-size))
(mini-bench (apply vector (iota bench-size)))
(mini-bench (apply u32vector (iota bench-size)))
(mini-bench (list->vlist (iota bench-size)))

(define bench-ivector (apply ivector (iota bench-size)))
;;(define bench-ivector-u32 (apply ivector-u32 (iota bench-size)))
(define bench-list (iota bench-size))
(define bench-vector (apply vector (iota bench-size)))
(define bench-u32vector (apply u32vector (iota bench-size)))
(define bench-vlist (list->vlist (iota bench-size)))

(newline)
(display "== benchmarking iteration...") (newline)
(mini-bench (ivector-fold + 0 bench-ivector))
;;(mini-bench (ivector-u32-fold + 0 bench-ivector-u32))
(mini-bench (fold + 0 bench-list))
(mini-bench (vector-fold + 0 bench-vector))
(mini-bench (vlist-fold + 0 bench-vlist))

(newline)
(display "== benchmarking iteration by index...") (newline)
(mini-bench (let iter ((i 0) (acc 0))
              (if (< i (ivector-length bench-ivector))
                  (iter (+ i 1)
                        (+ acc (ivector-ref bench-ivector i)))
                  acc)))
#;(mini-bench (let iter ((i 0) (acc 0))
              (if (< i (ivector-u32-length bench-ivector-u32))
                  (iter (+ i 1)
                        (+ acc (ivector-u32-ref bench-ivector-u32 i)))
                  acc)))
(mini-bench (let iter ((i 0) (acc 0))
              (if (< i (vector-length bench-vector))
                  (iter (+ i 1)
                        (+ acc (vector-ref bench-vector i)))
                  acc)))
(mini-bench (let iter ((i 0) (acc 0))
              (if (< i (u32vector-length bench-u32vector))
                  (iter (+ i 1)
                        (+ acc (u32vector-ref bench-u32vector i)))
                  acc)))
(mini-bench (let iter ((i 0) (acc 0))
              (if (< i (vlist-length bench-vlist))
                  (iter (+ i 1)
                        (+ acc (vlist-ref bench-vlist i)))
                  acc)))

(newline)
(display "== benchmarking concatenation...") (newline)
(mini-bench (ivector-append bench-ivector bench-ivector))
;;(mini-bench (ivector-u32-append bench-ivector-u32 bench-ivector-u32))
(mini-bench (append bench-list bench-list))
(mini-bench (vector-append bench-vector bench-vector))
(mini-bench (vlist-append bench-vlist bench-vlist))
