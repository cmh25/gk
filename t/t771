/ bughunt round 9 (2026-07-16): f32 sentinel lexing.
/
/ An `e`-suffixed float sentinel (0ne/0ie/-0ie) followed with NO space by a
/ sign or digit used to mis-lex: the suffix `e` was grabbed as an exponent
/ introducer, so `0ne+5` read as 0n^(e+5) -> an f64 NaN, and the `+` operator
/ vanished (a NaN/inf has no exponent, so the `e` can only be the type suffix).
/ Fix: lex.c gn_ sets `sente` on the 0n/0i sentinel path and skips the
/ exponent branch of state 8 there.
0ne+5.0e
4:0ne+5.0e
0ie+5.0e
0ne-3.0e
0ne*2.0e
0ie-2.0
4:0ie-2.0
0ne+0.0e
/ real scientific notation is unaffected (no sentinel mantissa)
1.5e10
1.5e10e
1e5
2.0e+3
4:2.0e+3
6.022e23
1.0e-5e
0.0e+5
4:0.0e+5
/ and the space/comma/semicolon separated forms always worked
0ne 1.0e
(0ne;5.0e)
/
/ exponent-vs-sentinel: a lone `0` glued to a sentinel letter (n/i/N/I) after
/ an exponent sign is the START of a sentinel, not an exponent digit -- so
/ `5.0e+0ne` is `5.0e + 0ne`, not `5.0e+0` with a dangling `ne`.  (0N/0I widen
/ into the f32 left operand via the round-8 sentinel-aware rule.)
5.0e+0ne
5.0e+0n
5.0e+0i
5.0e-0i
5.0e+0N
5.0e+0I
1.0e+0ne
/ multi-digit exponents are untouched: no valid number literal changes.
/ (5.0e+10n = 5e10 then a dangling `n`; 5.0e00n = 5.0 then `n`.)
5:5.0e+0
5:5.0e+10
5:5.0e-05
5:5.0e0
5:6.022e23
5:1.5e10e
/ the one IRREDUCIBLE ambiguity, resolvable only with spaces: a plain digit
/ after the exponent sign is a genuine exponent (maximal munch), so `1.0e+2`
/ is 100.0; write `1.0e + 2` for the f32-plus-int reading (3.0e).
1.0e+2
1.0e + 2
\\
