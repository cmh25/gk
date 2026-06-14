\l load.k
\d accel
A:3 4#1 2 3 4 5 6 7 8 9 10 11 12.0e
B:4 2#1 2 3 4 5 6 7 8.0e
"A:";A
"B:";B
"accelerate mm[A;B]:";mm[A;B]
"cpu mul[A;B]:"; mul[A;B]
"match small:"; (mm[A;B])~mul[A;B]
/ bigger square case
n:128
M1:(n,n)#(n*n)#1 2 3 4 5 6 7.0e%7
M2:(n,n)#(n*n)#7 6 5 4 3 2 1.0e%3
"match 128x128:"; (mm[M1;M2])~mul[M1;M2]
/ non-square: 100x64 * 64x40
P:(100,64)#(6400)#1 2 3.0e%3
Q:(64,40)#(2560)#3 2 1.0e%2
"match 100x64*64x40:"; (mm[P;Q])~mul[P;Q]
\\
