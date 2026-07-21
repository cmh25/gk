/ A dict built incrementally (dset doubles the key/value allocation, so its
/ capacity slot outgrows the count) used to corrupt the heap after a db/bd
/ roundtrip.  The wire ships slot 2 verbatim -- the SENDER's capacity, e.g. 64
/ for 41 keys -- but the receive side rebuilds the vectors exactly count-sized.
/ dset's append then trusted the stale capacity, skipped its resize, and wrote
/ one past the 41-element vector: a heap overflow (AFL sig 11, 2026-07-14; a
/ hostile blob could make the capacity arbitrary).  db now resets slot 2 to the
/ real key count on arrival.

/ incremental build -> capacity outgrows count; roundtrip; append past the end
dd:.+(,`a;,0)
i:0;while[i<40;@[`dd;`$"n",$i;:;i];i:i+1]
#!dd
dd0:db bd dd
dd0~dd
#!dd0
dd0[`n5]
dd0[`z]:99
dd0[`z]
#!dd0

/ many appends past the roundtrip drive the resize path repeatedly
i:0;while[i<40;@[`dd0;`$"m",$i;:;i];i:i+1]
#!dd0
dd0[`m39]
dd0[`n5]

/ a flip-built dict (capacity already == count) roundtrips and appends too
ee:.+(`$"k",'$!40;!40)
#!ee
ee0:db bd ee
ee0~ee
ee0[`k17]
ee0[`z]:7
#!ee0
ee0[`z]
\\
