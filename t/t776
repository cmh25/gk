/ $-parse and long-literal sentinel/saturation consistency
/ - 0$ and 0j$ accepted only bare digit strings; now 0$ takes an optional
/   leading -, surrounding blanks, and the 0N/0I spellings ("+5", inner
/   junk, and floats stay domain errors).
/ - xatoi saturated low overflow to INT32_MIN, which IS 0N -- now the
/   lexer saturates to -0I (the null pattern is reserved for the 0N
/   spelling and parse failure).  xatol/strtoll had the same one-off at
/   INT64_MIN, reachable from j literals: -99999999999999999999j was 0Nj.
/ - 0.0e$ parsed with the f64 reader, so "3.5e" and "0ie" both came back
/   0ne; xstrtoe takes the f32 spellings the lexer takes and the printer
/   emits.  "5e" is 5 then the name e -- not a literal, not parseable
/   (the trailing e must come through a decimal point or exponent).
/ - float-side parse failure was a silent null (0.0$"12x" was 0n); it is
/   now a domain error.  Empty stays the null.
/ Same readers back the 0: fixed-width I/J/E fields.
0$"-5"
0$" 12 "
0$"-2147483648"
0$"-4000000000"
0$"4000000000"
0$"0N"
0$"0I"
0$"-0I"
0$""
0j$"-5"
0j$"0N"
0j$"0I"
0j$"-0I"
0j$"-99999999999999999999"
0j$"99999999999999999999"
0.0$" 1.5 "
0.0$"0i"
0.0$"-0i"
0.0$"0n"
0.0e$"3.5"
0.0e$"3.5e"
0.0e$"5.0e"
0.0e$"1e0e"
.[$;(0.0e;"5e");:]
0.0e$"0ne"
0.0e$"0ie"
0.0e$"-0ie"
0.0e$"0n"
0.0e$"2.147484e+09e"
.[$;(0.0;"12x");:]
0.0$""
0.0e$""
/ long literals: INT64_MIN, exact or via strtoll saturation, becomes -0Ij,
/ never 0Nj -- matching the i32 clamps (-2147483648 is -0I)
-99999999999999999999j
-9223372036854775808j
-9223372036854775807j
-99999999999999999999 2j
9223372036854775807j
/ the formatting direction of $ is unaffected
5$"abc"
3$123
\\
