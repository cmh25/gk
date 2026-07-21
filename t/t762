/ db/bd roundtrip of an EMPTY dict, then append (AFL sig 11, 2026-07-14).
/ db resets the capacity slot to the key count on arrival (the t758 fix),
/ which for an empty dict is 0 -- and dset's grow step doubled it: 0<<1
/ stayed 0, kresize rebuilt the key/value vectors zero-sized, and the
/ append wrote past them (heap overflow).  The grow step now treats a 0
/ capacity as "start fresh at DMAX" like dnew.
d:db bd .()
d[`z]:7
d[`z]
#!d
d[`y]:8
d~db bd d
/ one key (capacity == count == 1) exercises the normal doubling next door
e:db bd .+(,`a;,1)
e[`b]:2
e[`a]+e[`b]
#!e
\\
