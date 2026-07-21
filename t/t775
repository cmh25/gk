/ Float promotion in vector literals goes by SPELLING, not bit pattern
/ (src/lex.c): a digit token keeps its text value when the vector becomes
/ float, even at the i32 sentinel boundaries, where
/ `2147483647 1 2.0` is 2.147484e+09 1 2, not 0i 1 2.  Before the fix the
/ two int->float promotion sites matched iv[i] against INT32_MAX/INT32_MIN/
/ INT32_MIN+1, so exactly three digit values transmuted into +-inf while
/ their neighbors (2147483646, 2147483648) kept their values.
2147483646 1 2 3.0
2147483647 1 2 3.0
2147483648 1 2 3.0
-2147483647 1 2 3.0
-2147483648 1 2 3.0
4000000000 1 2 3.0
/ digit strings past int64 range promote by their text, not strtoll's clamp
99999999999999999999 1 2 3.0
/ same boundaries arriving after the float token (second promotion site)
1.0 2147483647
1.0 -2147483647
1.0 -2147483648
/ sentinel spellings still widen by meaning
0N 1 2 3.0
0I 1 2 3.0
-0I 1 2 3.0
1.0 0N
1.0 0I
1.0 -0I
/ f32 vectors: same rule
2147483647 1 2 3.0e
0I 1 2 3.0e
0N 1 2 3.0e
/ negative zero still takes the vector's type
-0 1.0
1.0 -0
-0 1
/ long vectors already promoted by spelling; unchanged
0I 2147483647 3j
0N 1 2 3j
-0I 1 2 3j
2147483647 1 2 3j
/ scalar and int-vector saturation unchanged: inside i32, the boundary
/ value IS the sentinel
2147483647
4000000000 1 2 3
\\
