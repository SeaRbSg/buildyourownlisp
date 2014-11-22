(def {nil} {})
(def {true} 1)
(def {false} 0)
(def {else} true)
(def {fun} (lambda {f b} {def (head f) (lambda (tail f) b)}))

(fun {unpack f l}
     { eval (join (list f) l) })

(fun {pack f & xs} {f xs})

(def {curry} unpack)
(def {uncurry} pack)

(fun {do & l}
     { (if (== l nil)
           {nil}
         {last l}) })

(fun {let b}
     { ((lambda {_} b) ()) })

(fun {not x}   {- 1 x})
(fun {or  x y} {+ x y})
(fun {and x y} {* x y})

(fun {flip f a b} {f b a})
(fun {ghost & xs} {eval xs})
(fun {comp f g x} {f (g x)})

(fun {first l}  { eval (head l) })
(fun {second l} { eval (head (tail l)) })
(fun {third l}  { eval (head (tail (tail l))) })

(fun {len l}
     { (if (== l nil)
           {0}
         {+ 1 (len (tail l))}) })

(fun {nth n l}
     { (if (== n 0)
           {first l}
           {nth (- n 1) (tail l)}) })

(fun {last l}
     { nth (- (len l) 1) l })

(fun {take n l}
     { (if (== n 0)
           {nil}
         {join (head l) (take (- n 1) (tail l))}) })

(fun {drop n l}
     { (if (== n 0)
           {l}
         {drop (- n 1) (tail l)}) })

(fun {split n l}
     { list (take n l) (drop n l) })

(fun {elem x l}
     { (if (== l nil)
           {false}
         {if (== x (first l)) {true} {elem x (tail l)}}) })

(fun {map f l}
     { (if (== l nil)
           {nil}
         {join (list (f (first l))) (map f (tail l))}) })

(fun {filter f l} 
     { (if (== l nil)
           {nil}
         {join (if (f (first l)) {head l} {nil}) (filter f (tail l))}) })

(fun {foldl f acc l}
     { (if (== l nil)
           {acc}
         {foldl f (f acc (first l)) (tail l)}) })

(fun {foldr f acc l}
     { (if (== l nil)
           {acc}
         {f (first l) (foldr f acc (tail l))}) })

(fun {sum l} {foldl + 0 l})
(fun {product l} {foldl * 1 l})

(fun {cond & cs}
     { (if (== cs nil)
           {error "No selection found"}
           {(if (first (first cs))
                {eval (second (first cs))}
                {unpack cond (tail cs)}) }) })

(fun {case x & cs}
     { (if (== cs nil)
           {error "No case found"}
           { (if (== x (first (first cs)))
                 {eval (second (first cs))}
                 {unpack case (join (list x) (tail cs))} ) }) })

(fun {fib n}
     { (cond
        {(== n 0) {0}}
        {(== n 1) {1}}
        {else {(+ (fib (- n 1))
                  (fib (- n 2)))} }) })
