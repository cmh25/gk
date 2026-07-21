/ db (deserialize) type-confusion via a trusted wire subtype (bughunt r7,
/ 2026-07-15).  deserialize ended in `r|(K)st<<48`, OR-ing the wire subtype
/ into the value.  s(x) is an 8-bit field, but st was read as a full i32 and
/ never bounded: a hostile blob (untrusted IPC body / db payload) with high
/ bits in st corrupted the TYPE field, forging e.g. an int atom into a fake
/ heap vector with an attacker-picked low-48 "pointer".  Found by fuzzing a
/ live IPC server's message body; the deserialized value crashed (k_ derefs
/ address 0x1) the moment a handler referenced it.  Fix: reject a wire
/ subtype outside 0..255, and guard the structural routes (fnrestore/legacy
/ converters) that read fixed slots of a crafted short list.
/
/ bh turns a hex string into the raw bytes; @[db;x;:] traps the deserialize
/ so a bare error doesn't halt -- (1;"type") means db REJECTED the blob.
/ Printing the trap result USES it: pre-fix db returned a poison K here and
/ printing it crashed, so this line is a real regression signal, not a
/ result-discarding no-op.
/
/ 1. int atom (t=1) blessed with an out-of-range subtype st=0x30e8
v:bh"0300000001000000e830000000000000000000000000"
@[db;v;:]
/ 2. a 1-slot list (t=0) blessed as a closure (st=0xc3); fnrestore needs the
/    real 4-slot lambda shape and would otherwise read past the list
w:bh"0300000000000000c30000000000000001000000000000000100000000000000000000002a000000"
@[db;w;:]
/ 3. an empty list blessed as a legacy arity-3+ projection (st=0xd5): the
/    n(r)-1 arity computation underflowed
z:bh"0300000000000000d500000000000000000000000000000000000000"
@[db;z;:]
/ valid payloads still deserialize / round-trip unchanged
u:bh"030000000100000000000000000000002a000000"
db u
(db bd 1 2 3)
(db bd `a`b`c)
(db bd {x+y})[3;4]
(db bd {[a;b]a*b})[5;6]
(db bd .((`a;1);(`b;2)))`b
g:db bd{c:x;{c+y}}[10];g[5;3]
\\
