/ projected arguments are captured as independent VALUES at formation --
/ never references ("never store a reference in a projected argument").
/ A live-tree capture (@[;].k) otherwise closes a refcount ring the
/ moment it is stored back into the tree (.k -> d -> P -> .k), which
/ refcounting can never reclaim; the fuzz crash behind this rule made the
/ closure exit gate walk that ring (the capture probe's depth guard lied
/ "captures", then the conversion walk's error unwind freed each ring
/ node once per visit -- UAF).  A shared dict arg is now kcp'd at every
/ projection formation site, mirroring dset's root rule, so the ring can
/ no longer be built; the exit-gate walks ALSO cut at any node already on
/ their own path, as defense in depth should a ring arrive another way.
/ Results PRINT, so a failing diff shows the actual value.

a0:1
p:@[;].k
a0:2
p`a0                     / freeze.apply   -> 1  the captured .k is a snapshot, not live
q:@[.k;]
a0:3
q`a0                     / freeze.bracket -> 2  bracket-elided capture snapshots too
w:@[;]@[;].k
a0:4
w`a0                     / freeze.nested  -> 3  nested projection froze at ITS formation
/ ordinary shared dicts: eager copy is observably identical to the old
/ lazy copy-on-write (the projection never saw later writes either way)
e:.,(`v;7;)
w2:@[;]e
e.v:8
w2`v                     / dict.arg.value -> 7  the projection kept its copy
e.v                      / dict.live      -> 8  the original still writes
/ storing tree-capturing projections into the tree no longer forms rings
/ (teardown reclaims everything -- the ASan/valgrind runs are the check)
d.b:@[;].k
d.c:@[;]@[;].k
#!d                      / stored         -> 2
d:0
/ the old fuzz shape: a frame-capturing lambda stored beside the (now
/ snapshotted) tree capture, converted by the exit gate on the way out
({c:5;.k.cy2.g:{c*x};.k.cy2.p:@[;].k;cy2}[]`g)3   / convert.beside -> 15
cy2:0
\\
