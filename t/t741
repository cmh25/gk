/ group (monadic =) on an int vector holding a SENTINEL (0I, 0N, -0I) next to
/ a normal value spun forever: the hash branch sized its table from the
/ min/max scan, which SKIPS sentinels -- so =0I,1 had more distinct values
/ than nonzero slots and the probe loop never found a free one.  The fuzzer
/ caught it (as `=72337203685477,1`: a bare over-2^31 literal clamps to 0I).
/ Same table math in groupj; distinct (?) already carried the m+=3 headroom.
/ Results PRINT, so a failing diff shows the actual value.

=0I,1                    / inf sentinel      -> (,0;,1)   (was: hung forever)
=0N,1                    / null sentinel     -> (,0;,1)
=-0I,1                   / -inf sentinel     -> (,0;,1)
=0I 0I                   / all sentinel      -> (,0 1)
=0N 0I -0I 1 1 0N        / mixed             -> (0 5;,1;,2;3 4)
=72337203685477,1        / the fuzz literal (clamps to 0I)  -> (,0;,1)
=2147483646,1            / just under the clamp             -> (,0;,1)
=1 2 1 3 1               / plain, no sentinel -> (0 2 4;,1;,3)
?0I,1                    / distinct already had the headroom -> 0I 1
\\
