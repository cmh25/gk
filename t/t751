/ 2026-07-13 long-index-vector amend: a[longidx]:v looped kamendi4v once
/ PER INDEX, re-walking the base each call -- quadratic whenever the value
/ forced a base retype (a[b-0j]:b-1.0 at 777k was a fuzz hang).  A narrow
/ long index vector now converts to int once and takes the vectorized path.
a:!9
a[(0j+2 4)]:7.5 8.5
a
4:a
b:0j+!9
b[(0j+1 3)]:0.5 0.25
b
c:!9
c[(0j+0 8)]:10 20j
c
4:c
d:!5
@[`d;(0j+1 2);+;1.0]
d
e:!5
e[(0j+!5)]:2 3 4 5 6
e
4:e
\\
