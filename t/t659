/ 64-bit int (long/j) -- dyadic arithmetic type matrix: + - * % ^
/ the printed suffix shows the result type: Nj=long, N.0=float, N=int

/ --- plus: atom x atom (8 8, 8 1, 1 8, 8 2, 2 8) ---
2j+3j
2j+3
2+3j
2j+3.0
2.0+3j
/ atom x vector and vector x atom
10j+1 2j
1 2j+10j
10j+1 2 3
1 2 3+10j
10j+0.5 1.5
0.5 1.5+10j
/ vector x vector (-8 -8, -8 -1, -1 -8, -8 -2, -2 -8)
1 2j+3 4j
1 2j+10 20
10 20+1 2j
1 2j+0.5 0.5
0.5 0.5+1 2j

/ --- minus ---
5j-2j
5j-2
5-2j
5j-2.0
2.0-5j
10 20j-1 2j
1 2 3-10j

/ --- times ---
3j*4j
3j*4
3*4j
3j*4.0
1 2 3j*10j
2.0*1 2 3j

/ --- divide (always float) ---
7j%2j
6j%2j
7j%2
7%2j
1 2 3j%2j
1j%0j
1j%0
0j%0j

/ --- power (always float) ---
2j^10j
2j^10
2^10j
2j^0.5
1 2 3j^2j

/ --- edge: 64-bit range, wrap, sentinels ---
1000000000j*1000000000j
3000000000j*3000000000j
0Ij+1j
0Nj-1j
-5j+2j
-5j*-5j
0Nj+1
0Ij+0.0
abs 0Nj
/ empty vectors
(0#0j)+0#0j
4:(0#0j)+0#0j

/ long atom/vector broadcast against a nested list (each adverb)
1 2j*(5 6j;7 8j)
1 2j*(5 6;7 8)
10j+(1 2 3;4 5j)
(5 6j;7 8j)*1 2j

\\
