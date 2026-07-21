/ A single-argument bracket index/apply whose body is a CLOSED term -- a paren
/ group (2) or an apply f[1] -- must not swallow a following dyad.  Regression
/ for the r013 bug where `V[(2)]+1` parsed as `V[(2)+1]` (-> V[3]=40 instead of
/ V[2]+1=31): a closed elist reached r013 un-wrapped (r017 only wraps t2/t1 into
/ a t3 elist), so with a trailing pz it fell through to the juxt branch.

V:10 20 30 40 50
W:0 1 2 3 4
f:{2*x}
M:(1 2 3;4 5 6;7 8 9)

/ ---- the bug: closed bracket body + trailing dyad ----
V[(2)]+1
f[(1)]+1
V[f[1]]+1
V[W[2]]+1
V[f[1]],5
V[f[f[1]]]+1
V[(f[1])]+1
V[(2)]*2+1

/ ---- must stay correct (non-closed body, or dyad on the left) ----
f[1]+1
V[f 1]+1
V[1*f[1]]+1
2+V[f[1]]
M[0;f[1]]+1
M[f[1];0]+1
(V[f[1]])+1

/ ---- plain apply / index unchanged ----
V[2]
V[(2)]
f[1]
V[2 3]
M[1;2]

/ ---- indexed assign & amend into a closed index ----
V[(2)]:99
V
@[V;(f 0);:;7]
\\
