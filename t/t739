/ a closure snapshot copies the LAMBDA FRAMES it captured -- not the namespace
/ scopes above them, and not the ktree.
/
/ scope_cp recursed all the way to the root, so every closure created a copy of
/ the whole global scope AND the ktree.  The lambdas inside that copy had scopes
/ pointing back into it, so refcounting could never free it (t254: "extra copy
/ of ktree was getting created and never freed").  The workaround was to register
/ each copy in scopea[] -- the table that exists to tear down NAMESPACE scopes at
/ exit -- which pinned a permanent reference (no closure snapshot was ever freed
/ while the process ran) and burned a slot per copy out of a fixed 1000.  When it
/ filled, gk printed "error: scope_new() i==SM" and called exit(1): untrappable,
/ no k error, exit code 0.  ~330 closures killed the interpreter.
/
/ Namespace scopes are shared instead: a closure must see a global's CURRENT
/ value, and scope_get_ refreshes a namespace scope's dict from the ktree on
/ every lookup anyway, so the copies were never even read.
/ Results PRINT, so a failing diff shows the actual value.

/ creating closures in bulk no longer exhausts the scope table (v4 died at ~330)
u:{c:x;{c*x}}
do[5000;u 3];
(u 3)4                   / -> 12   (was: exit(1) "scope_new() i==SM" at ~330)
/ ...and the ones we keep still work
v:u 3
v 4                      / -> 12
(v 4;(u 4)4)             / -> 12 16

/ a captured global stays LIVE -- it is not frozen into the snapshot
g:1
ug:{c:x;{c+g}}
vg:ug 10
b4:vg[]
g:2
(b4;vg[])                / -> 11 12  the same closure sees the NEW g
/ ...and a captured LOCAL is frozen, as it must be
uc:{c:x;{c+0}}
v1:uc 1
v2:uc 2
(v1[];v2[])              / -> 1 2

/ the closure identity still holds across the usual shapes
mk:{c:x;{c+:1;c}}
{(x[];x[];x[])}mk 10     / -> 11 12 13  a mutable counter
({x[4]})v                / -> 12   passed to another fn
{c:5;f:{c*x};f}[][3]     / -> 15   a sibling lambda returned directly
({c:x;{d:x;{c+d+x}}7}3)1 / -> 11   two enclosing frames
/ two closures from the same fn do not share state
c1:mk 100
c2:mk 200
c1[];c1[];
(c1[];c2[])              / -> 103 201

/ the t254 shape (a lambda returning a lambda, applied by symbol) still behaves
f:{{j}}
h:f`
@[h;`;:]                 / -> (1;"value")
\\
