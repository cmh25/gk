/ `f _ x` with a float f: round x to a multiple of f (down for f>0, up for f<0)
/ the multiples used to be walked one at a time from j=1, so +-inf never
/ terminated, every x below f collapsed to 0.0, and the atom cases ignored f.

/ +-inf and NaN have no multiples -- pass them through, don't spin
0.5_,0i
0.5_,-0i
0.5_,0n
-0.5_,0i
-0.5_,-0i
0.5_,0I               / int +inf sentinel widens to +inf
0.5_,-0I
0.5_,0N

/ negative x rounds like positive x (used to collapse to 0.0)
0.3_,-1.0
0.3_,-7.3
0.3_(-1.0 -7.3)
-2.0_,-7.3
2.0_,-7
-2.0_,-7
1.0_,-0.5
-1.0_,0.5
-1.0_,0.0

/ -0.0 keeps its sign (1% reveals it: 1%-0.0 is -0i)
1%0.5_,-0.0
1%0.0_,-0.0
1%0.5_,0.0

/ atom agrees with its own one-element vector
(2.3_20.0)~*2.3_,20.0
(-2.3_20.0)~*-2.3_,20.0
(0.3_1.0)~*0.3_,1.0
(2.0_7.3)~*2.0_,7.3
(2.0_7)~*2.0_,7
2.3_20.0
-2.3_20.0
0.3_1.0

/ f==0 is the identity; a non-finite f has no multiples
0.0_7.3
0.0_,7.3
0i_,7.3
0n_,7.3
-0i_,7.3

/ exact multiples and fp boundaries stay put
0.5_(-1.0 -2.0)
0.1_,0.3
0.2_,0.6
0.3_1.0 2.0 3.0
0.5_1 2 3

/ closed form, not a walk: these would need 1e15 / 1e300 steps
1e-9_1000000.0
1e-300_1.0

/ a SUBNORMAL f makes x/f overflow to +-inf before the floor; the guard returns
/ x (multiples of f are denser than the fp spacing near x), not a spurious +-inf
1e-309_1.0
-1e-309_1.0
1e-309_,-1.0
1e-320_1.0
\\
