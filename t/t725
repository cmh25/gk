/ A lambda (or builtin) head with an adverb, juxtaposed onto an already-
/ adverbed application, must compose: f'g'x == f'(g'x).  Regression for the
/ c3_apply dyadic-juxt detection popping a pending head's bare adverb (0x85,
/ or an 0x45/0x43 adverb node) off the operand stack as an each-both left
/ seed -> type error.  A real noun (or lambda) below stays a seed, so
/ `1 g'!5` and the `f g'x` valence error are unchanged.

f:{1+x}
g:{2*x}
h:{x+y}

/ ---- the bug: adverbed head over an adverbed application ----
f'g'1 2 3
f'f'g'1 2 3
h/g'1 2 3
h\g'1 2 3
sqr'g'1 2 3
mul/g'1 2 3
f'sqr'1 2 3
{x,x}'g'1 2

/ ---- seeded / left-noun forms (gk each-right semantics) ----
10 h/g'1 2 3
10 h/(g'1 2 3)
1 h'!3
1 2 3 h'10 20 30

/ ---- parenthesized equivalents unchanged ----
f'(g'1 2 3)
h/(g'1 2 3)
sqr'(g'1 2 3)
\\
