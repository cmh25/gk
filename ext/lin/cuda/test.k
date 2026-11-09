\l load.k
\d cuda

\t ginit`
A:(1 2 3.0e;4 5 6.0e;7 8 9.0e)
gmul[A;A]
f:(_1e6)?1.0e
A:100 100#f
\t gmul[A;A]
A:1000 1000#f
\t gmul[A;A]

/ exec[f;X]: a scalar lambda is the kernel, one thread per element; exec[f;(X;Y)] a 2D grid, one thread per (x;y)
!info[]
n:10000000
v:n?100000000
w:n?1000
g:v%3
k:3
(exec[{x*x+1};v])~v*v+1                                    / 1D, int
(exec[{x*x-1.5};g])~g*g-1.5                                / float
(exec[{v[x]+w[x]};!n])~v+w                                 / index space: v w are free vectors
(exec[{v[x]*w[x]+1};!n])~v*w+1                             / f'[v;w] is the index form
(exec[{x*k};v])~v*k                                        / a free atom is a uniform
(exec[{((x*x)+(2*x)+1)%(x+1)};g])~((g*g)+(2*g)+1)%(g+1)    / a fused chain: one kernel vs 6 cpu passes
(exec[{[i](v[i]!7)&(w[i]>500)};!n])~(v!7)&(w>500)          / explicit param, mod, min, compare
h:{a:v;b:w;exec[{a[x]+b[x]};!n]}                           / the enclosing function's locals are the lambda's free names
(h[])~v+w
m:{[c] exec[{c*v[x]};!n]}                                  / a param as the uniform
(m[5])~5*v
p:3000?100
q:2000?100
(exec[{x*y};(p;q)])~{x*q}'p                                / 2D grid: the outer product, a (#p;#q) matrix
(exec[{p[x]+q[y]};(!#p;!#q)])~{x+q}'p                      / 2D index space
r:7?100
(exec[{x+y+z};(5#p;6#q;r)])~{[u]{u+x+r}'6#q}'5#p           / 3D grid: a list of matrices
u:n?100
(exec[{+/x};u])~+/u                                        / reductions: a per-block tree in shared memory, then the partials
t:n?10
(exec[{+/x*x};t])~+/t*t                                    / the map under the / runs per thread
(exec[{|/x};v])~|/v
(exec[{&/x};v])~&/v
o:1+n?2
(exec[{*/x};o])~*/o
(exec[{+/u[x]*w[x]};!n])~+/u*w                             / dot product, index form
1e-9>abs((exec[{+/x};g])-+/g)%+/g                          / float sum: a different order of additions, so relative error, not ~
stats[]                                                    / compile h2d kernel d2h total (ms), n
\t r:exec[{x*x+1};v]                                       / gpu incl copies
\t r:v*v+1                                                 / cpu primitive
\t r:{x*x+1}'v                                             / cpu interpreted each
\t r:exec[{((x*x)+(2*x)+1)%(x+1)};g]
\t r:((g*g)+(2*g)+1)%(g+1)
\t r:exec[{x*y};(p;q)]
\t r:{x*q}'p
\t r:exec[{+/x*x};t]
\t r:+/t*t
src[{x*y};(p;q)]                                           / the generated CUDA source

\\
