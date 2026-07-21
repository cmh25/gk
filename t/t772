/ bughunt round 9 (2026-07-16): shift/rot follow the universal
/ "either operand i64 -> i64" promotion rule, like and/or/xor/div/gcd/lcm and
/ every arithmetic primitive.  Previously the result width came from the LEFT
/ (value) operand alone, so 1 shift 40j masked to a 32-bit shift (256j->256).
/ Now an i64 COUNT promotes the result to i64; the value widens sentinel-aware
/ (0N shift 3j == 0Nj shift 3).  Same-width behavior is unchanged.
/ scalar: i64 count promotes
1 shift 40j
4:1 shift 40j
(1 shift 40j)~1j shift 40
1 rot 40j
(1 rot 40j)~1j rot 40
/ same-width i32 unchanged (masked 32-bit)
1 shift 40
4:1 shift 40
1 shift 32
1 shift 32j
/ vectors
1 2 shift 40j
4:1 2 shift 40j
1 2 shift 3 4j
1 2 rot 3 4j
4:1 2 rot 3 4j
/ sentinel value widens like arithmetic
(0N shift 3j)~0Nj shift 3
(-1 shift 3j)~-1j shift 3
/ i64 value with i32 count already promoted (unchanged)
4:1j shift 40
\\
