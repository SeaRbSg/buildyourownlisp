(def {fun} (lambda {args body} {def (head args) (lambda (tail args) body)}))

({add-together})
(fun {add-together x y} {+ x y})
(add-together 2 3)

({unpack})
(fun {unpack f xs} {eval (join (list f) xs)})
(fun {pack f & xs} {f xs})
(def {uncurry} pack)
(def {curry} unpack)

(curry + {5 6 7})
(uncurry head 5 6 7)

(def {add-uncurried} +)
(def {add-curried} (curry +))
(add-curried {5 6 7})
(add-uncurried 5 6 7)
