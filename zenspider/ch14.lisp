"Hello"
"hello\n"
"hello\""
(head {"hello" "world"})
(eval (head {"hello" "world"}))

; comment

(def {nil} {})

(+ 1 0)                                 ; add
(cons 1 nil)                            ; cons
(def {a} 42)                            ; def
(/ 628 2)                               ; div
(eval {(/ 628 2)})                      ; eval
(== 42 (+ 41 1))                        ; eq
;; skipping -- error
(^ 41 40)                               ; exp
(>= 5 2)                                ; ge
(> 5 2)                                 ; gt
(head (list 1 2 3))                     ; head
(if (> 5 2) {1} {0})                    ; if
(join (list 1 2) (list 3 4))            ; join
                                        ; lambda
(<= 2 5)                                ; le
(len (list 1 2 3))                      ; len
(list 1 2 3)                            ; list
(load "ch12.lisp")                      ; load
(< 2 5)                                 ; lt
(== 3 (max 1 2 3))                      ; max
(min 1 2 3)                             ; min
(% 9 2)                                 ; mod
(* 157 2)                               ; mul
(!= 314 3)                              ; ne
(print "Woot")                          ; print
;; (= a {24})                              ; put =
(read "(+ 1 2 3)")                      ; read
(- 4 2 1)                               ; sub
(tail (list 1 2 3))                     ; tail
