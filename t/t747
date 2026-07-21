/ 2026-07-13 numeric-domain property-hunt fixes (proptest/numeric.py round).
/ Results PRINT, so a failing diff shows the actual value.

/ --- N1: lsq/inv on a matrix with MIXED int/float rows.  Every other la
/ entry (lu/qr/svd/mul/det/rref) already accepted them; lsq flips its
/ design matrix, and mixed rows became type-0 columns -> type error.
inv(2.0 1 1;4 3 3;8 7 9)
lsq[1.0 2 3;(2.0 1 1;4 3 3;8 7 9)]
lsq[1.0 2 3;(2 1 1j;4 3 3;8 7 9)]
inv(2 1 1j;4.0 3 3;8 7 9)
inv(2 1 1;4 3 3;8 7 9)

/ --- N2: %empty lost the result TYPE: () instead of 0#0.0 (the -1/-2 arms
/ route through each(), which returns an untyped () for empty input; the
/ long/real arms were already typed)
4:%!0
4:%0#0.0
%!0
4:%0#0j
4:%0#2.0e

/ --- N3: shape of a ragged list: an atom ends the dimension, but children
/ already enqueued from EARLIER siblings kept the walk alive and emitted a
/ phantom dimension (^((,0;,1);-17) gave 2 1)
^((,0;,1);-17)
^((1 2;3 4);7)
^(7;(1 2;3 4))
^(1 2 3;4)
^((1 2;3 4);(5 6;7 8))
^3 3#!9

/ --- N4: floor of -infinity (and beyond-int64 negatives) clamped to 0N,
/ one below -0I; +infinity already clamped to 0I.  Now symmetric on the
/ int arms (f64 + f32, atom + vector) and J_NINF on the long arm.
_ -0i
_ 0i
_ 0n
_(-0i;0i;0n)
_-0ie
_-1e300
_1e300
_-3e9
_1 2.7 -3.2
\\
