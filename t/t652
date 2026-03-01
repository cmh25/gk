/ int literal underflow and 0N parsing (lex.c)
/ -2147483648 and below (numeric) -> -0I; only 0N/-0N literals -> 0N
/ 0N in float vector (e.g. 0n 0N) -> 0n 0n

/ scalar: negative underflow literals display as -0I
-2147483647
-2147483648
-2147483649
-2147483650

/ scalar: 0N and -0N literals stay 0N
0N
-0N

/ vector: mix of 0N literal and underflow
0N -2147483648 1
-2147483649 0N -2147483650

/ float vector with int 0N (0n 0N -> 0n 0n)
0n 0N
0i 0I
-0i 0I

\\
