/ 2026-07-13 group/unique table hashing: the multiplicative hash kept only
/ LOW product bits, so integer-valued doubles (zero low mantissa),
/ stride-2^k ints/longs, and (via khash) digit STRINGS all collapsed into
/ one probe chain -- quadratic.  =5.0-!100000 and grouping 100k formatted
/ strings were fuzz hangs (>60s); the scale lines below run in ~ms now and
/ hang the suite visibly if the complexity regresses.
#=5.0-!200000
#?5.0-!200000
#=$!100000
#?$!100000
#=0.0e+!100000
#=65536j*!200000
#=65536*!100000
/ semantics pinned: +-0.0 share a group, first-seen sign survives ?,
/ general lists group type-strictly, sentinel hashing intact
=0.0,-0.0
?-0.0 0.0
?0.0 -0.0
=0.0 5 -0.0 5
=(5;5j;5.0;5.0e)
=5.0 5 5.0e 5
?0n 0n 0i 0i -0i -0i
?0I 0I 0N 0N -0I -0I
\\
