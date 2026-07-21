/ db (deserialize): a subtype must ride the base type serialize emits it on
/ (bughunt r7 follow-up, 2026-07-16; AFL fuzzer01 sig:11, a havoc mutation of
/ t766 case 2).  t766 added per-subtype SHAPE guards, but each one was keyed on
/ the base type -- the structural checks under `t==0`, the index/name checks
/ under `t>0`.  A hostile blob that pairs a subtype with the WRONG base type is
/ not caught by those guards, it simply MISSES them: every check is skipped and
/ the value still gets blessed at `r|(K)st<<48`.  Deserializing 0xc0..0xdc x
/ every base type crashed in 68 combinations.  Fix: pin the base type for every
/ subtype up front (default arm rejects), so each later guard is only ever
/ reached on the shape it was written for.
/
/ bh turns a hex string into the raw bytes; @[db;x;:] traps the deserialize
/ so a bare error doesn't halt -- (1;"type") means db REJECTED the blob.
/ Printing the trap result USES it: pre-fix db returned a poison K here and
/ printing it crashed, so these lines are real regression signals, not
/ result-discarding no-ops.
/
/ 1. float atom (t=2) blessed as a closure (0xc3) -- the AFL crash: kprint_'s
/    closure arm read the double's bits as a ko* and dereferenced NULL
a:bh"0300000002000000c3000000000000000000000000000000"
@[db;a;:]
/ 2. int atom (t=1) blessed as a 2:-linked C function (0xdc): the 0xdc reject
/    was `t==0`-gated, so an ATOM rebuilt the arbitrary-call primitive --
/    linkcall does g=(fn)jk(pf[0]); g(...) on an attacker-chosen payload
b:bh"0300000001000000dc0000000000000000000000"
@[db;b;:]
/ 3. the mirror image: an empty LIST (t=0) blessed as an int-index builtin
/    (0xc6), which skipped the `t>0` BMONAD bound check -> apply CALLs a
/    function pointer read off the end of the table
c:bh"0300000000000000c6000000000000000000000000000000"
@[db;c;:]
/ 4. char atom (t=3) blessed as a dict (0x80): dget/atcb read fixed slots
d:bh"0300000003000000800000000000000041"
@[db;d;:]
/ 5. symbol atom (t=4) blessed as an (f;args) wrapper (0xd9)
e:bh"0300000004000000d9000000000000006100"
@[db;e;:]
/ 6. int vector (t=-1) blessed as an (f;av) wrapper (0xda)
f:bh"03000000ffffffffda00000000000000010000000000000007000000"
@[db;f;:]
/ 7. 0xdb retired in Pass 1b (replaced by 0xda): no consumer, so reject
g:bh"0300000000000000db000000000000000000000000000000"
@[db;g;:]
/ 8. 0xd4 (legacy arity-2 projection) is a pure subtype REBRAND to 0xd9, but it
/    converted-and-returned before 0xd9's arity guard: an empty list became a
/    0-slot 0xd9 whose consumers read slots 0,1 -> heap over-read (ASan).  The
/    rebrand now falls through to that guard, so both spellings are covered.
i:bh"0300000000000000d4000000000000000000000000000000"
@[db;i;:]
/    ...and a 1-slot 0xd4: still short of the (verb;args) pair 0xd9 requires
j:bh"0300000000000000d400000000000000010000000000000001000000000000000000000005000000"
@[db;j;:]
/ valid payloads still deserialize / round-trip unchanged, on every base type
/ a subtype legitimately rides
u:bh"030000000100000000000000000000002a000000"
db u
(db bd 1 2 3)
(db bd `a`b`c)
(db bd "hello")
(db bd 1.5 2.5)
(db bd (1;`a;"s";2.5;()))
(db bd 5j)
(db bd 1.5e)
(db bd +)[2;3]
(db bd 2+)3
(db bd +/)1 2 3 4
(db bd {x*2}')1 2 3
(db bd {x+y})[3;4]
(db bd {[a;b]a*b})[5;6]
(db bd .((`a;1);(`b;2)))`b
h:db bd{c:x;{c+y}}[10];h[5;3]
\\
