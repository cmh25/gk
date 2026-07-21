/ APPLY_N inline bracket-apply arg release: the borrowed-args cleanup loop
/ after fne_fast whitelisted heap types by hand (vectors, float atoms) and
/ missed boxed longs (type 8), so every f[x] with a long-atom arg leaked one
/ ref on the value (x:5j;g:{x};g[x] leaked x's value once per call).  The
/ predicate is now kh().  Values were always correct -- this pins the
/ expressions for the ASan sweeps that see the refcount imbalance.
x:5j
g:{x}
g[x]
g[x]+g[x]
h:{x+y}
h[x;x]
do[3;g[x]]
x
g[7j]
y:2.5
g[y]
v:1 2 3j
g[v]
z:5
g[z]
\\
