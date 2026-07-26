/ Width-family fixes from the 2026-07-25 audit (batch 3).
/ (The dyadic `5:` boundary case that belongs with these lives in t64/t43: it
/ needs a count of 2147483646, which the ASan build's VSIZE/VLEN cap at 1e9
/ rejects, so it cannot run under `make testa`.)

/ `^` small-scale must be untouched by the general-list arm's widening
/ (its dimension array went u32 -> i64 and gained the BIGV dispatch).
^"abc"
^,"abc"
^(1 2 3;4 5 6)
^((1 2;3);(4;5))
^3 3#!9
^!0
^((^-18);,16)
^((,0;,1);-17)
\\
