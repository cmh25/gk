/ bughunt round 8 (2026-07-16): language-consistency fixes
/
/ 1. `at` on a LIST used to conform over the LEFT argument (irecur2), so
/    (1 2;3 4) at 1 gave the COLUMN 2 4 and (1;"a") at 0 was a rank error;
/    at is an index (a conforms over x only), with nul as the OOB fill.
(1 2;3 4)at 1
(1;"a")at 0
(1;"a";2)at 0 5 2
(kv 1 2 3)at 0 5
/ dicts: @ already null-fills a missing key; at now routes there
(.+(`a`b;1 2))at `b
nul~(.+(`a`b;1 2))at `zz
(.+(`a`b;1 2))at `a`zz`b
/ symbol handle derefs (read side of pass-by-reference), OOB fills
t770v:10 20 30
`t770v at 1
`t770v at 0 2 9
/ undefined handle -> nul, the identity vector
`t770nosuch at 7
/ non-integer index stays a type error; atom left stays rank
@[{(1;"a")at x};"x";:]
@[{5 at x};0;:]
/
/ 2. i64 vector literals: 0I/-0I now adopt the vector's width like 0N does
/    (ref.md: "Int nulls and infinities ... adopt like any int")
0N 0I -0I 2j
0I 2j
2j -0I
/
/ 3. sentinel-aware widening: i32->i64 and i32->f32 conversions map
/    0N/0I/-0I to the target type's sentinels (as i32->f64 always did)
0N&1j
0I+0j
-0I+0j
0I|5j
0N=0Nj
0I=0Ij
-0I<0j
0N<-5000000000j
0N+0.0e
0I+0.0e
-0I*1.0e
(0N 0I -0I 1)+0j 0j 0j 0j
(0N 0I -0I 1)*1.0 1.0 1.0 1.0e
0N!5j
/ i32->f64 (the long-standing behavior, unchanged)
0N+0.0
0I+0.0
/
/ 4. f32 prints with up to 9 sig digits under \p (was capped at 7, so some
/    f32 values could never round-trip as text); default \p 7 is unchanged
5:0.1e*802
\p 17
5:0.1e*802
(. 5:0.1e*802)~0.1e*802
5:1.0000001e
(. 5:1.0000001e)~1.0000001e
\p 7
5:0.1e*802
\\
