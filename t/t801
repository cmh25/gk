/ indexed compound assignment is a deep amend, including elided path levels,
/ shape-changing verbs, Cartesian-product paths, and repeated indices
d:4#,,1
r:d[;0],:2
(r;d)

d:!10
r:d[],:1
(r;d)

d:3#,3#,!3
r:d[0 1;1 2;0 1],:5
(r;d)

a:1 2
r:a[0 0]+:1 1
(r;a)

/ plain indexed assignment returns the post-write selection, including
/ broadcast and duplicate-index last-write behavior
a:!5
r:a[]:9
(r;a)

a:!5
r:a[1 2]:9
(r;a)

a:!3
r:a[0 0]:8 9
(r;a)

d:4#,,1
r:d[;0]:2
(r;d)

d:3#,3#,!3
r:d[0 1;1 2;0 1]:5
(r;d)

/ :: changes assignment scope but has the same post-write selected result
a:!5
r:a[]::9
(r;a)

a:!3
r:a[0 0]::8 9
(r;a)

d:4#,,1
r:d[;0]::2
(r;d)

a:!3
f:{r:a[]::7;(r;a)}
f`
a

/ the same leaf-amend and return rule is shared by the compound verbs
a:!5
r:a[1 2]-:1
(r;a)

a:1 2 3
r:a[]*:2
(r;a)

/ typed vectors take their normal amend path and retain type in the result
a:1 2 3j
r:a[0 2]:9j
(r;a)

a:1 2 3j
r:a[]+:10j
(r;a)

a:1.0 2.0 3.0
r:a[0 2]:9.5
(r;a)

a:1.0e 2.0e 3.0e
r:a[]+:0.5e
(r;a)

a:"abc"
r:a[0 2]:"XY"
(r;a)

a:`a`b`c
r:a[0 2]:`x`y
(r;a)

/ mixed and nested lists preserve boxing across return and write-back
a:(1;"ab";`c)
r:a[0 2]:(9;`z)
(r;a)

a:((1 2;3 4);(5 6;7 8))
r:a[;0],:9
(r;a)

/ dictionaries as targets and as assigned values
d:.+(`a`b;(1 2j;3 4j))
r:d[;1]+:10j
(r;d)

v:.+(`a`b;1 2)
a:2#0
r:a[0]:v
(r@`a;a[0]@`b)

/ copy-on-write and returned selections survive later rebinding
a:(1 2;3 4)
b:a
r:a[;0]+:10
(r;a;b)

a:(1 2;3 4)
r:a[]:9
a:0
r

/ lambdas and projections remain callable after assignment and rebinding
a:2#0
r:a[0]:{x+1}
(r 4;a[0] 5)

p:{x+y}[10;]
r:a[1]:p
(r 4;a[1] 5)
a:0
r 6

/ projection through a shape-changing compound amend
p:{x+y}[10;]
a:2#,!0
r:a[],:p
(r[0;0]4;a[1;0]5)
a:0
r[1;0]6

/ projection through global indexed assignment
a:2#0
f:{r:a[1]::p;(r 4;a[1]5)}
f`
a[1]6

/ lambdas and projections assigned together through dictionary indexing
d:.+(`f`p;(0;0))
q:{x+1}
r:d[`f`p]:(q;p)
(r[0]4;r[1]4;d.f 5;d.p 5)
d:0
r[1]6

/ primitive projections remain callable after assignment and rebinding
p:+[10;]
a:,0
r:a[0]:p
(r 4;a[0]5)
a:0
r 6

/ named-builtin projections (gk extensions), including a retained list argument
p:mul[3;]
a:,0
r:a[0]:p
(r 4;a[0]5)
a:0
r 6

p:at[10 20 30;]
a:,0
r:a[0]:p
(r 1;a[0]2)
a:0
r 0

/ closed lambdas retain captured values after assignment and rebinding
mk:{c:x;{c+x}}
q:mk 10
a:,0
r:a[0]:q
(r 5;a[0]6)
a:0
r 7

/ projections of closed lambdas retain both their capture and bound argument
mkp:{c:x;{[a;b]c+a+b}[10;]}
q:mkp 100
a:,0
r:a[0]:q
(r 4;a[0]5)
a:0
r 6

/ a primitive projection and closure assigned together through dictionary indexing
p:*[3;]
q:mk 20
d:.+(`p`q;(0;0))
r:d[`p`q]:(p;q)
(r[0]4;r[1]4;d.p 5;d.q 5)
d:0
(r[0]6;r[1]6)
\\
