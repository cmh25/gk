/ db + index: two AFL crashes (fuzzer01 sig:11 stack-overflow, fuzzer06 sig:11
/ heap-buffer-overflow), both in src/b.c, 2026-07-17.
/
/ 1. db (deserialize) 0xc5 composition: a real composition is a 2-slot list
/    (slot 0 the list of composed fns, slot 1 the adverb char-vector), and BOTH
/    kprint_ and fe.c's av-peel read slot 1.  The shape guard only required
/    >=1 slot, so a 1-slot 0xc5 whose element is a list (to clear the T==0
/    check) passed, then printing the poison value read px[1] off the end --
/    heap-buffer-overflow READ at kprint_ (k.c:492), "0 bytes after 8-byte
/    region".  Guard now pins the exact 2-slot shape.  @[db;v;:] -> (1;"type")
/    means db REJECTED the blob; printing the trap result USES it, so this is a
/    real regression signal, not a result-discarding no-op.
v:bh"0300000000000000c50000000000000001000000000000000000000000000000000000000000000000000000"
@[db;v;:]
/ valid compositions still round-trip unchanged (positive control)
(db bd (+/-)) 6 2
(db bd (|+/)) 1 2 3
(db bd (+/#)) 1 2 3 4
/
/ 2. at_ symbol-handle deref (b.c): a handle can deref to another handle, so
/    the chain is runtime data, not syntactic nesting -- a cycle recursed
/    forever with no depth guard -> stack-overflow.  Now bounded by the shared
/    maxr/stack_low guard, so a cyclic handle trips a (trappable) stack error.
/    Self-cycle:
a:`a
@[{a at 0};0;:]
/    Mutual cycle b->c->b:
b:`c
c:`b
@[{b at 0};0;:]
/ valid handle indexing (single deref, and a 2-hop chain) still works
w:10 20 30
h:`w
h at 1
g:`h
g at 2
\\
