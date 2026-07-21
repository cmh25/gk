/ global assign (:: and the compound forms) targets the namespace the function
/ was DEFINED in -- not the caller's \d, and not the discarded call frame.
/ K manual: "if b is a global variable and the expression b :: y appears in a
/ function expression, the effect [is] to assign the contents of y to the
/ global variable b in the same directory as where the function is defined.
/ (However, if the definition also contains an ordinary assignment b: z then
/ b will be local)."
\d foo
c:10
amd:{c+:1;c}         / compound assign to a home global
wr:{c::x;c}          / :: to a home global
mk:{zz::x}           / :: to a name that exists NOWHERE
rd:{c+x}             / read side (already worked)
lcl:{c:5;c::6;c}     / a local of the same name shadows the global
dp:{d[`k]::x}        / d[`a]:: to a home global dict
d:.+(,`k;,1)
\d .k
/ read resolves in the fn's home namespace
foo.rd 5                 / -> 15
/ the counter advances -- was 11,11,11 with the write discarded
foo.amd[]                / -> 11
foo.amd[]                / -> 12
foo.amd[]                / -> 13
foo.c                    / -> 13   the home global really moved
@[{c};0;:]               / -> (1;"value")   and nothing leaked to root
/ :: writes home and is visible to the next read
foo.wr 99;
foo.c                    / -> 99
@[{c};0;:]               / -> (1;"value")
/ a name that exists nowhere is created in the fn's home namespace
foo.mk 7;
foo.zz                   / -> 7
@[{zz};0;:]              / -> (1;"value")
/ a name that exists ONLY in the caller's namespace still writes home
rr:1
\d foo
mk2:{rr::x}
\d .k
foo.mk2 8;
foo.rr                   / -> 8   created in foo, not written to the root rr
rr                       / -> 1   the caller's rr is untouched
/ indexed :: into a home dict
foo.dp 5;
foo.d[`k]                / -> 5
/ a local of the same name shadows the global (both ways)
g:7
h:{g:5;g::6;g}
h[]                      / -> 6   the local
g                        / -> 7   the global is untouched
foo.lcl[]                / -> 6
foo.c                    / -> 99  foo.c still holds the :: write
/ closures still mutate their captured frame, not a namespace
u:{v:x;{v+:1;v}}
w:u 10
w[]                      / -> 11
w[]                      / -> 12
w[]                      / -> 13
/ a fn defined at root, called while \d foo is current, writes ROOT (its home),
/ not the caller's current namespace
q:0
bump:{q+:1;q}
\d foo
.k.bump[];
.k.bump[];
\d .k
q                        / -> 2   both bumps landed at root
@[{foo.q};0;:]           / -> (1;"value")   nothing leaked into foo
/ strict lexical scope: the write target is fixed where the fn was DEFINED.
/ The caller's namespace never follows the call -- not when called from another
/ namespace, not when the fn is passed to a fn in another namespace and invoked
/ there, and not when another namespace is the current \d.
\d .k.ns1
n:0
bmp:{n+:1;n}
\d .k.ns2
n:100                / ns2 has its own n -- must never be touched by ns1.bmp
call:{x[]}
own:{n+:1;n}
\d .k
ns1.bmp[];
(ns1.n;ns2.n)            / -> 1 100
ns2.call ns1.bmp;    / fn passed into ns2 and invoked there
(ns1.n;ns2.n)            / -> 2 100
\d .k.ns2
.k.ns1.bmp[];        / invoked while ns2 is the current \d
\d .k
(ns1.n;ns2.n)            / -> 3 100
ns2.own[];
(ns1.n;ns2.n)            / -> 3 101  ns2's own fn writes ns2
\\
