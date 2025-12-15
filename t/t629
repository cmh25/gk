/ UB regression test - exercises undefined behavior fixes

/ 1. Signed integer overflow in arithmetic (PMT macro)
2147483646+1
2147483646+2
2147483646+3
0-2147483647-2
1 2 3+2147483646

/ 2. Float hashing with NaN/inf (unique, group)
?0n 0n 0i 0i -0i -0i
?0I 0I 0N 0N -0I -0I
=0n 0i -0i 0n 0i

/ 3. Sorting with extreme values (sort.c max-min+1)
a:0I 0N -0I 5 -5
a@<a
a@>a

/ 4. floor of infinity/nan (floor__)
_ 0i
_ -0i
_ 0n
_ 0i 0n -0i

/ 5. Accumulator overflow in scan/over (av.c)
+\2147483645 1 1 1 1 1 1
+/2147483645 1 1 1 1 1 1
*\1 2 2147483647
*/ 2 2 2147483647

/ 6. Integer vector arithmetic overflow
1 2147483646 3+1 1 1
2147483647 1*2 1

\\
