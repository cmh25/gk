/ aliased dotted-path amend: copy-on-write must relink the copied node into its
/ parent, else the shared original is under-counted and double-freed (afl crash
/ {x:e:b.c:.`;b.c.e:0}0) and the amend leaks. Also assert COW leaves the other
/ aliases of the shared subtree untouched.
{x:e:b.c:.`;b.c.e:0}0
a.x:5
b:a
b.x:9
a.x
b.x
p.q.r:1
s:p
s.q.r:2
p.q.r
s.q.r
\\
