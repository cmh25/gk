/ a closure escapes in ANY returned shape.  The old return trigger only
/ converted a bare 0xc3/0xd9 or an all-lambda dict, so `{c:5;f:{c*x};,f}`
/ (and any list/mixed-dict/nested return) came out pointing at the dead
/ frame.  Now the frame carries a flag -- "a lambda parents here", set by
/ fnd/fncp_ at creation, the only two parentage creators -- and a flagged
/ frame's return is walked whole, whatever its shape.  Frames that define no
/ local lambda skip everything, so ordinary code pays nothing.
/ Results PRINT, so a failing diff shows the actual value.

/ list returns
(*{c:5;f:{c*x};,f}[])3                  / enlist       -> 15  (was: dead frame)
q:{c:5;f:{c*x};(f;42)}[]
((*q)3;q 1)                             / mixed.list   -> 15 42
(*|*{c:7;f:{c+x};((1;f);2)}[])3         / nested       -> 10
/ a literal never assigned to any local still converts (the flag is set at
/ parse time, not at assignment)
(*{c:5;{,{c*x}}[]}[])3                  / literal      -> 15
/ a dict mixing data and one captured lambda (the all-lambda rule missed it)
d:{c:5;f:{c*x};.((`f;f);(`d;42))}[]
(d[`f]3;d`d)                            / mixed.dict   -> 15 42
/ a projection inside a returned list
(*{c:5;f:{c*x+y};(f 1;0)}[])2           / proj.in.list -> 15

/ a LAMBDA projection is a noun, exactly like the lambda itself: a verb on
/ its left applies instead of forming a train, so `,(f 1)` ENLISTS (it used
/ to compose into a 0xc5), and `-` type-errors on both a lambda and a lambda
/ projection alike
4:,{x+y}[1]                             / proj.noun       -> 0  (a list)
#,{x+y}[1]                              / proj.noun.count -> 1
@[{-{x+y}};0;:]                         / negating a lambda            -> type
@[{-{x+y}[1]};0;:]                      / ...and a lambda projection   -> type
/ ...and the enlisted capturing projection converts like any list element
(*{c:5;f:{c*x+y};,(f 1)}[])2            / enlist.proj    -> 15
(*db bd {c:5;f:{c*x+y};,(f 1)}[])2      / bd.enlist.proj -> 15

/ escaped under collector churn
u:{c:x;f:{c*x};{f x}}
lq:{c:5;f:{c*x};,f}[]
do[600;u 0]
(*lq)3                                  / escape       -> 15

/ recursion and siblings through a list-returned closure
lr:{f:{$[x<1;0;x+f x-1]};,f}[]
(*lr)4                                  / rec.in.list  -> 10
lm:{f:{x*2};g:{f x};(g;f)}[]
((*lm)21;(*|lm)5)                       / mutual.in.list -> 42 10

/ the wire carries all of it
(*db bd {c:5;f:{c*x};,f}[])3            / bd.enlist       -> 15
dd:db bd {c:5;f:{c*x};.((`f;f);(`d;42))}[]
(dd[`f]3;dd`d)                          / bd.mixed.dict   -> 15 42
(*db bd {f:{$[x<1;0;x+f x-1]};,f}[])4   / bd.rec.in.list  -> 10

/ a frame with a local lambda returning plain data converts nothing (the
/ walk finds no capture; the value comes back untouched)
{f:{x};(1 2 3;"abc")}[]                 / data.ret     -> (1 2 3;"abc")
\\
