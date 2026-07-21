/ a lambda captured in a closure snapshot can see the frame it lives in:
/ closure_siblings (fn.c) re-parents the snapshot's dict lambdas to the
/ snapshot, so a sibling reached through an inner closure, a RECURSIVE local
/ f, and mutually-recursive siblings all resolve -- all three value-errored
/ before (local recursion and mutual recursion simply did not work in gk).
/ Each back-reference closes a refcount ring the cycle collector (cc_sweep)
/ frees once nothing outside the ring remains; the collector must never free
/ a ring that is still reachable, so the escape shapes below hammer it with
/ sweep-triggering churn before using their closures.
/ Results PRINT, so a failing diff shows the actual value.

/ the three shapes
{c:5;f:{c*x};{f x}}[]3                  / sibling    -> 15  (was: value error)
{f:{$[x<1;0;x+f x-1]};{f x}}[]4         / recursion  -> 10  (was: value error)
{f:{x*2};g:{f x};{g x}}[]5              / mutual     -> 10  (was: value error)

/ still working: sibling returned directly, and one that needs nothing local
{c:5;f:{c*x};f}[]3                      / direct     -> 15
{c:5;f:{x*2};{f x}}[]3                  / nolocal    -> 6

/ dict-of-lambdas closures see their siblings too
dd:{c:5;f:{c*x};.((`a;{f x});(`b;{c}))}[]
(dd[`a]3;dd[`b][])                      / dict       -> 15 5

/ deeper local recursion across collector sweeps (churn registers + frees
/ rings).  Depth 30 stays inside the ASan build's maxr=100 depth cap.
u:{c:x;f:{c*x};{f x}}
do[600;u 0]
{f:{$[x<1;0;x+f x-1]};{f x}}[]30        / deep       -> 465

/ escaped sibling: the lambda fished out of the snapshot keeps its whole ring
/ alive -- these were heap-use-after-free repros under the borrowed-parent
/ attempt, and must stay correct under collector churn
w:{c:5;f:{c*x};{f}}
h:w[]
g:h[]
h:0
do[600;u 0]
g 3                                     / escape.ret     -> 15
w2:{c:5;f:{c*x};{g2::f}}
v:w2[]
v[];
v:0
do[600;u 0]
g2 3                                    / escape.assign  -> 15

/ live closures survive sweeps; dead rings are collected (no leak: see the
/ RSS recipe in the fn.c comment -- 100k iterations stay flat)
cl:u'!50
do[600;u 0]
+/{x 2}'cl                              / live       -> 2450

/ a sibling under a projection/adverb WRAPPER is re-parented too (fn_inner):
/ the snapshot's values get the same wrapper treatment the returned value
/ always had via closure_any
{c:5;f:{c*x+y};g:f 1;{g x}}[]2          / proj         -> 15
{c:2;f:{c*x};g:f';{g x}}[]1 2 3         / adverb       -> 2 4 6
{f:{$[y<1;0;y+f[x;y-1]]};g:f 0;{g x}}[]4  / proj.rec   -> 10
{c:5;f:{c*x+y};{f[1]x}}[]2              / proj.inline  -> 15

/ escaped wrapped sibling: the projection fished out of the snapshot keeps
/ its ring alive across collector churn
wp:{c:5;f:{c*x+y};g:f 1;{g}}
hp:wp[]
kp:hp[]
hp:0
do[600;u 0]
kp 2                                    / escape.proj   -> 15

/ a lambda hiding in a projection's HELD ARGS is converted too -- the whole
/ conversion surface: args directly, via a global whose function slot
/ captures nothing, nested in a list, nested in a dict, and self-application
A2:{x y}
{c:1;g:{x y};h:{c+x};g[h]}[]3           / args          -> 4
{c:1;h:{c+x};A2[h]}[]3                  / args.global   -> 4
{c:1;h:{c+x};{(*x)y}[,h]}[]3            / args.list     -> 4
{c:1;h:{c+x};{x[`f]y}[.(,(`f;h))]}[]3   / args.dict     -> 4
{f:{$[y<1;0;y+x[x;y-1]]};f[f]}[]4       / args.self     -> 10
/ escaped held-arg projection across collector churn
pa:{c:1;g:{x y};h:{c+x};g[h]}[]
do[600;u 0]
pa 3                                    / args.escape   -> 4

/ bd/db round-trip: a deserialized closure recurses and sees siblings, live
/ semantics exactly (fnrestore parents captured lambdas to the frozen frame;
/ fncap ships ring members env-less instead of recursing around the ring --
/ wrapped ring members keep their wrapper, env-less inside: fncap_strip)
(db bd {c:5;f:{c*x};{f x}}[])3          / bd.sibling      -> 15
(db bd {f:{$[x<1;0;x+f x-1]};{f x}}[])4 / bd.recursion    -> 10
(db bd {f:{x*2};g:{f x};{g x}}[])21     / bd.mutual       -> 42
(db bd {c:5;f:{c*x+y};g:f 1;{g x}}[])2  / bd.proj         -> 15
(db bd {c:2;f:{c*x};g:f';{g x}}[])1 2 3 / bd.adverb       -> 2 4 6
(db bd {c:5;f:{c*x+y};{f[1]x}}[])2      / bd.proj.inline  -> 15
(db bd {c:1;g:{x y};h:{c+x};g[h]}[])3   / bd.args         -> 4
(db bd {c:1;h:{c+x};{(*x)y}[,h]}[])3    / bd.args.list    -> 4
(db bd {c:1;h:{c+x};{x[`f]y}[.(,(`f;h))]}[])3   / bd.args.dict  -> 4
(db bd {f:{$[y<1;0;y+x[x;y-1]]};f[f]}[])4       / bd.args.self  -> 10
\\
