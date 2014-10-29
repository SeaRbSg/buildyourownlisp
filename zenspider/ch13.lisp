== 1 (== 1 1)
== 0 (== 1 2)
== 1 (!= 1 2)
== 0 (!= 1 1)
(if (== 1 1) {1} {0})
(if (== 1 2) {0} {1})
(if (!= 1 2) {1} {0})
(if (!= 1 1) {0} {1})

def {fun} (lambda {args body} {def (head args) (lambda (tail args) body)})
(fun {len l} { if (== l {}) {0} {+ 1 (len (tail l))} })
(fun {reverse l} { if (== l {}) {{}} {join (reverse (tail l)) (head l)} })

len {1 2 3}
reverse {1 2 3}
