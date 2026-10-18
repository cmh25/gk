/ symbol identity (=, ?, group, find, lin, ~, with the empty symbol and dups),
/ the radix tier for wide-range longs, small-vector float grades (8-bit digit
/ path) and the branchless fused filter at 0 / all / 50% / last-element
/ selectivity in every payload width.
s:`a`bb``ccc`a``bb
s=`a
s=`
`=s
s=`a`bb``ccc`a``bb
?s
=s
#'=s
s?`bb
s?`
s?`zz
`a`bb lin s
s lin `a`
`a lin s
(`a`b)~`a`b
(`a`b)~`a`c
(`a`b)<`a`c
(`b`a)>`a`a
`a<`b
`b=`b
w:100000#`x`y`z`x
#'=w
?w
#&w=`x
(w@<w)~w@<w
j:-5 3 0Ij -0Ij 0 3 -5 0Nj 12j
<j
>j
j@<j
(<0j+!10)~!10
(>0j+!10)~|!10
v:(1000j*-7+!2000)*4000000001j
(<v)~<0.0+v
(>v)~>0.0+v
u:(1000003!7919*0j+!_300000)*3000000000j
(u@<u)~u@>u
(<u)~<0.0+u
(>u)~>0.0+u
f:3.5 0n -2.5 0.0 -0.0 3.5 0n 1e300 -1e300 7
<f
>f
f@<f
f@>f
e:0.0e*f
<e
>e
z:1000#1 2 3.0
(<z)~<1000#1 2 3
(>z)~>1000#1 2 3
y:70000#1 2 3.0
(<y)~<70000#1 2 3
(>y)~>70000#1 2 3
x:1000?1.0
(x@<x)~|x@>x
+/(1_x@<x)<-1_x@<x
x2:1000?1.0e
(x2@<x2)~|x2@>x2
n:!20
n@&n<0
n@&n<100
n@&n>9
n@&n=19
n@&~n<10
c:"abcdefghij"
c@&(!10)>4
c@&(!10)<0
c@&(!10)=9
g:0.0+!20
g@&g<10.5
g@&g>18.5
h:0j+!20
h@&h>14
h@&h<0
r:0.0e*!20
r@&r<3.0
sy:`a`b`c`d`e
sy@&(!5)>2
sy@&(!5)<9
sy@&(!5)>9
sy@&(!5)=4
(0.0+!1000)@&(!1000)=999
+/(!100000)@&(!100000)<50000
+/(0j+!100000)@&(!100000)>49999
/ float floor: fused single pass + branch-free int clamp; sentinels, both
/ int32 edges, promotion to long, real input
_ 1.5 -1.5 0.0 -0.0 0n 0i -0i 2.9999999 -2.0000001
_ 2147483646.5 -2147483647.5 1.0
_ 2147483647.0 1.0
_ -2147483648.0 1.0
_ -2147483647.0 1.0
_ 1e300 -1e300 1.0
_ 9223372036854775808.0 -9223372036854775809.0 1.0
_ 1.5 2.5 3.5e
_ 3000000000.0e 1.5e
_ 0.5*!10
+/_ 0.001*!100000
/ dotted / namespace paths (ktree_get path cache): absolute, relative, deep,
/ re-resolved after the node is replaced, and a 9-component path (uncached)
.k.zz1:5
.k.zz1
zz1
a9.b.c:1
a9.b.c
a9.b.c:2
a9.b.c
a9
.k.a9.b.c
.k.a9:`x
.k.a9
q1.q2.q3.q4.q5.q6.q7.q8.q9:7
q1.q2.q3.q4.q5.q6.q7.q8.q9
.k.q1.q2.q3.q4.q5.q6.q7.q8.q9
/ symbols a verb manufactures must be the interned null symbol
`=*0#`
`~*0#`
(*0#`)=`
(`a;`)?*0#`
e3:3#0#`
e3=`
=e3
?e3
#'=e3
e3?`
` lin e3
\\
