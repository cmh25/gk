/ bd/db carry a closure's captured environment.
/
/ A lambda's scope slot[0] is the environment its free variables resolve
/ against.  The scope itself is not shippable -- it chains up through the
/ globals into the ktree, and is cyclic -- so bd used to write `null` there,
/ and db handed back a lambda whose captured names were simply GONE: a silently
/ broken function that value-errored on use.  ref.md promises db bd x round-
/ trips any K value and IPC ships "any K value", so a closure sent to a peer
/ arrived broken.
/ Now bd flattens the captured bindings (the enclosing LAMBDA frames on the
/ chain, inner shadowing outer, stopping at the first namespace scope) into a
/ dict, and db rebuilds them as a frozen parent frame.  Globals are NOT shipped:
/ they still resolve against the receiver, as they always did.
/ Results PRINT, so a failing diff shows the actual value.

/ the basic round-trip -- this used to be a value error
u:{c:x;{c*x}}
v:u 3
v 4                      / -> 12   the live closure
(db bd v)4               / -> 12   (was: value error, c was gone)
((db bd v)5;(db bd v)6)  / -> 15 18  a round-tripped closure is reusable
(db bd db bd v)4         / -> 12   and it round-trips again
/ several captured names
u2:{a:x;b:y;{a+b+x}}
v2:u2[10;20]
(db bd v2)1              / -> 31
/ two enclosing frames
u3:{c:x;{d:x;{c+d+x}}7}
v3:u3 3
(db bd v3)1              / -> 11
/ an inner name shadows an outer one
u4:{c:x;{c:99;{c+x}}[]}
v4:u4 3
(db bd v4)1              / -> 100  the inner c (99) wins, not the outer (3)
/ the captured value may itself be a function, or another closure
u5:{f:{x*2};{f x}}
(db bd u5[])5            / -> 10
mk:{c:x;{c*x}}
u6:{g:mk 3;{g x}}
(db bd u6[])4            / -> 12
/ a mutable counter keeps counting from the snapshot it was shipped with
mc:{c:x;{c+:1;c}}
k:mc 10
{(x[];x[])}db bd k       / -> 11 12
/ a big captured value survives
big:{c:x;{+/c}}[!10000]
(db bd big)[]            / -> 49995000
/ a frame that holds the very lambda being captured is a cycle; it is skipped
/ (the lambda is already the thing being serialized) rather than shipped inside
/ its own environment
sr:{c:5;f:{c*x};f}
(db bd sr[])3            / -> 15
/ NB a SIBLING lambda reached through an inner closure (`{c:5;f:{c*x};{f x}}`)
/ value-errors on c in the LIVE interpreter too, with or without a round-trip --
/ a separate, pre-existing closure bug -- so there is nothing for bd/db to
/ preserve there and it is not asserted here.
/ globals are NOT captured -- they resolve on the receiving side
g:100
u7:{c:x;{c+g+x}}
(db bd u7 3)1            / -> 104

/ --- nothing else about serialization moved ---
/ a plain lambda captures nothing, so its bytes are unchanged
(db bd {x+y})[3;4]       / -> 7
(bd{x+y})~bd{x+y}        / -> 1   the BYTES are unchanged (a raw byte string, so
                         /        this one stays a comparison)
(db bd mul[2;])5         / -> 10  a projection
(db bd(-%))2             / -> -0.5  a verb train
db bd 1 2 3
db bd .+(`a`b;1 2)
db bd"abc"
db bd(1 2;"ab";`c)
/ hex and compressed paths carry closures too (they wrap bd)
(db bh hb bd v)4         / -> 12
(db bz zb bd v)4         / -> 12
\\
