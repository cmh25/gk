\l show.k

c:10

d:.()
d.a:7 2 7 6 6 3 6 2 5 8
d.b:7 1 4 1 1 5 5 7 1 6
d.c:7 9 2 7 6 1 5 0 8 2

e:.()
e.a:6 8 6 0 8 2 1 2 1 9
e.b:6 6 3 4 4 7 3 2 8 7
e.c:6 6 5 4 5 2 8 6 6 0

"d"
if[c<101;show d]
"e"
if[c<101;show e]

"select"
i:&2=d`a
t:.()
t.a:d[`a;i]
t.b:d[`b;i]
t.c:d[`c;i]
if[c<101;show t]

"sort"
i:<d.a
t:.()
t.a:d[`a;i]
t.b:d[`b;i]
t.c:d[`c;i]
if[c<101;show t]

"tabulate"
g:=d`a
r:+/'g
t:.()
t.a:d[`a;*'g]
t.c:r
if[c<101;show t]

"link"
t:.()
t.a:d`a
t.b:d`b
t.c:d`c

i:e.a?/d`a
eb:e[`b],nul
ec:e[`c],nul
t.eb:eb[i]
t.ec:ec[i]
if[c<101;show t]

"willbe"
t:d
t[`d]:t[`a]%t`b
if[c<101;show t]

\\
