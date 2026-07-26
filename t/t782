/ Sentinel/widening sweep from the 2026-07-25 audit (batch 4).  gk documents
/ (ref.md, Numeric Type Rules) that a widening conversion maps a narrow type's
/ null/infinities to the wide type's; these sites used plain casts instead, so
/ a sentinel arrived as the ordinary number -2147483648 / 2147483647.

/ 1. `_` promote boundary was asymmetric: f>=INT32_MAX was right, f<INT32_MIN
/ missed BOTH negative sentinels, so a value that merely LOOKED like one came
/ back as one.
_ 2147483647.0
_ -2147483646.0
_ -2147483647.0
_ -2147483648.0
_ -2147483649.0
_ 1.0 -2147483648.0
_ -9223372036854775808.0 30000000000.0

/ 2. dyadic a$x printed the raw bit pattern; monadic $ always had the spellings.
20$0N
20$0I
20$-0I
20$0n
20$0i
10$0Nj
10$0Ij
20$123
$0N
$0I

/ 3. float mod / power domain / bin / div-or-xor / gcd-lcm: plain casts meant a
/ narrow sentinel became a finite number.  Each must now agree with the
/ same-width spelling on the line after it.
0N!2.5
0Nj!2.5
.[{x^y};(0N;0.5);:]
.[{x^y};(0n;0.5);:]
1e10 2e10 bin 0I
1e10 2e10 bin 0i
1e10 2e10 binl 0N 0I
1e10 2e10 binl 0n 0i
0N or 7j
0Nj or 7j
0I div 1j
0Ij div 1j

/ 4. jd decomposed a sentinel into a plausible-looking day number; dj (the
/ inverse) already mapped them to 0N.
jd 0N
jd 0I
jd -0I
dj 0N
jd 20231225
dj 0

/ 5. sv: a negative last digit sign-extended into the subtype field, so s(x)
/ read 0xFF and the multi-radix form type-errored while the single-radix
/ form worked.
10 10 sv 0 -1
10 sv 0 -1
24 60 60 sv 1 2 -3

/ 6. la mul fast path widened an int row with a plain cast; it runs precisely
/ when a float operand makes the widening rule apply.
mul[(0N 1;1 1);(1.0 0.0;0.0 1.0)]
(0N 1) dot 1.0 0.0

/ controls
7!2.5
1 3 5 bin 4
5 or 3
mul[(1 2;3 4);(1.0 0.0;0.0 1.0)]
\\
