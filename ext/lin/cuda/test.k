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

\\
