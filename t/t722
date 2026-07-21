/ numeric vector literals: an unsuffixed token has no type of its own and takes
/ the vector's, so the width is only known at the end or at the first suffix.
/ A `j` suffix is not a shape but an assertion, and no float vector can hold a
/ long, so mixing `j` with any float token is a parse error.  Use (1j;2.0).

/ ---- unsuffixed ints adopt ----
1 2 3
4:1 2 3
1 2 3j
4:1 2 3j
2j 3 4
4:2j 3 4
1 2.0
4:1 2.0
1 2 3.0e
4:1 2 3.0e
2200000000 1j
4:2200000000 1j

/ ---- unsuffixed decimals adopt: f64 unless an `e` appears, anywhere ----
1.0 2 3
4:1.0 2 3
1.0 2 3.0e
4:1.0 2 3.0e
1.2e 3.0 4
4:1.2e 3.0 4
1.2e 3.0
4:1.2e 3.0
1.5 2.5 4.0e
4:1.5 2.5 4.0e

/ ---- int nulls/infinities are unsuffixed too, so they adopt ----
0N 2j
4:0N 2j
0N 2.0
4:0N 2.0
0N 2.0e
4:0N 2.0e
1 0Nj
4:1 0Nj

/ ---- float sentinels are float tokens; f32 sentinels are `e`-suffixed ----
0n 2.0e
4:0n 2.0e
0ne 2.0
4:0ne 2.0

/ ---- `j` + any float token is a parse error ----
@[.;"1j 2.0";:]
@[.;"2.0 1j";:]
@[.;"1j 2.0e";:]
@[.;"1.2e 3j";:]
@[.;"-0j 1.0";:]
@[.;"0n 2j";:]
@[.;"0i 2j";:]
@[.;"0Nj 2.0";:]
@[.;"0ne 2j";:]
@[.;"1.0 0Nj";:]
@[.;"1 2j 3.0";:]
@[.;"1j 2 3.0e";:]

/ ---- but j with ints, and floats without j, are fine ----
@[.;"1 2 3j";:]
@[.;"0N 2j";:]
@[.;"1.2e 3.0 4";:]
@[.;"1 2.0";:]

/ ---- atoms are never affected ----
1j
4:1j
2.0
4:2.0
1.5e
4:1.5e
0Nj
4:0Nj

/ ---- a mixed list is the honest spelling ----
(1j;2.0)
4:(1j;2.0)
(1j;2.0e)
(1.2e;3.0)
\\
