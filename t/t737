/ a global reference inside a function belongs to the namespace the function was
/ DEFINED in -- for the by-reference amend family too, and the watch that guards
/ it must fire for those writes.
/
/ The read side already resolved through the fn's scope chain (`v 1 inside a
/ `\d ns` fn found ns.v from anywhere), but the amend family resolved its symbol
/ against gs -- the CALLER's namespace.  Called from root it looked up a `v` that
/ was not there, an undefined handle is nul, and amending nul ranked out.
/ And scope_set_ gated the watch trigger on s==gs, so once the write correctly
/ landed in the fn's own namespace the variable changed with its watch silent.
/ Results PRINT, so a failing diff shows the actual value.

\d .k.ns
v:1 2 3
d:.+(`a`b;1 2)
rd:{`v 1}                / read side (already worked)
w1:{@[`v;0;:;9];v}       / @[`sym;...] amend
w2:{.[`v;,1;:;8];v}      / .[`sym;...] amend
w3:{@[`v;0;+;10];v}      / amend with a verb
w4:{@[`d;`a;:;7];d[`a]}  / amend a dict by reference
\d .k

/ read resolves in the fn's home namespace, from another namespace
ns.rd[]                  / -> 2
/ every by-reference amend writes the fn's OWN namespace
ns.w1[]                  / -> 9 2 3   (was: 1 2 3, the write ranked out on nul)
ns.v                     / -> 9 2 3   the write really landed in ns
ns.w2[]                  / -> 9 8 3   .[`sym;...]
ns.w3[]                  / -> 19 8 3  amend with a verb
ns.w4[]                  / -> 7       a dict amended by reference
ns.d[`a]                 / -> 7       and it landed in ns.d
/ nothing leaked into the caller's namespace
@[{v};0;:]               / -> (1;"value")
@[{d};0;:]               / -> (1;"value")

/ the same amends, from a fn defined at ROOT, still write root
rv:1 2 3
rw:{@[`rv;0;:;9];rv}
rw[]                     / -> 9 2 3
rv                       / -> 9 2 3
/ and at top level
tv:1 2 3
@[`tv;0;:;5];
tv                       / -> 5 2 3

/ --- watch fires for a write into ANY namespace, not just the current one ---
\d .k.ns2
c:0
hit:0
wv:1 2 3
whit:0
f:{c::x}
g:{@[`wv;0;:;x];wv}
watch(`c;"hit::hit+1")
watch(`wv;"whit::whit+1")
\d .k
ns2.f 5;
ns2.hit                  / -> 1       the watch fired (was: silent)
ns2.c                    / -> 5
ns2.g 9;
ns2.whit                 / -> 1       and it fires for a by-reference amend too
ns2.wv                   / -> 9 2 3

/ a root watch still fires from a root fn
rc:0
rhit:0
watch(`rc;"rhit::rhit+1")
rf:{rc::x}
rf 5;
rhit                     / -> 1
/ a LOCAL of the watched name must NOT fire
lc:0
lhit:0
watch(`lc;"lhit::lhit+1")
lf:{lc:99;lc}
lf[];
lhit                     / -> 0
/ a fully-qualified write still fires
qc:0
qhit:0
watch(`qc;"qhit::qhit+1")
.k.qc:7
qhit                     / -> 1
\\
