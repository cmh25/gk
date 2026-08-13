/ Function values must have spelling-independent construction and a display
/ which reparses to the same behavior.

/ F1: an inline lambda is a noun on the left of a dyad.  +, * and | used to
/ disappear here; all four projections now construct and fail alike.
{x}+
{x}-
{x}*
{x}|
@[{({x}+)1 2 3};0;:]
@[{({x}-)1 2 3};0;:]
@[{({x}*)1 2 3};0;:]
@[{({x}|)1 2 3};0;:]
{x}-*
3{x}-*

/ F2: a minus-headed composition must not print as a negative literal.
neg:-(2+)
neg
neg 3 1 2
negs:$[neg]
negs
neg2:. negs
neg2
neg2 3 1 2
neg~neg2

/ F3: grouping a train, or reaching it through a name, must construct the
/ same left projection and report the error only when that value is applied.
pa:(<2+)+
base:<2+
pb:base+
pa
pb
pa~pb
@[pa;3 1 2;:]
@[pb;3 1 2;:]

/ @ applied to a fixed-left dyad is ordinary function application in every
/ spelling, including when the @ itself is first projected.
aa:(2+)@
two:2+
ab:two@
aa
ab
aa 3 1 2
ab 3 1 2
aas:$[aa]
aa2:. aas
aa2
aa2 3 1 2
aa~aa2

/ F4: adverb-derived function values compose across the same runtime boundary
/ as projections and trains.  Fold gives a scalar (so grade later ranks),
/ while scan provides a directly observable successful composition.
cf:<(+/)
fold:+/
cg:<fold
cf
cg
cf~cg
@[cf;3 1 2;:]
@[cg;3 1 2;:]
cs:<(+\)
scan:+\
ct:<scan
cs
ct
cs 3 1 2
ct 3 1 2

/ F5: these two values used to print and compare identically even though their
/ applications differ.  Both displays must independently round-trip.
tf:2+,
pg:(2+),
tf
pg
tf~pg
tf 1 2 3
pg 1 2 3
tfs:$[tf]
pgs:$[pg]
tfs
pgs
tf2:. tfs
pg2:. pgs
tf2
pg2
tf2 1 2 3
pg2 1 2 3
tf~tf2
pg~pg2
tf2~pg2

/ Parentheses put a callable in the noun slot of a following dyad; bare
/ verb syntax still forms/applies the train.  Fixed-left values must follow
/ the same rule as primitive and multi-verb values.
(+),3
(2+),3
(2+*),3
,(2+)3
twofn:2+
(twofn),3
(2+) . 3
twofn . 3
pure:+*
purepg:(+*),
purepg
purepg 3
purepgs:$[purepg]
purepg2:. purepgs
purepg2
purepg2 3
purepg~purepg2

/ Function-valued projection operands other than 0xd0 need the same explicit
/ boundary.  Cover a bracket projection and a builtin value.
right:+[;2]
atright:right@
atright
atright 3 1 2
atrights:$[atright]
atright2:. atrights
atright2
atright2 3 1 2
atright2s:$[atright2]
atright2s
atrights~atright2s
builtin:abs
atbuiltin:builtin@
atbuiltin
atbuiltin -3 2
atbuiltins:$[atbuiltin]
atbuiltin2:. atbuiltins
atbuiltin2
atbuiltin2 -3 2
atbuiltin~atbuiltin2

/ A monadic builtin under over/scan has an optional do/while controller.
/ Its explicit projection spelling must retain that derived-verb wrapper.
dosqrt:sqrt/[5;]
dosqrt
dosqrt 2.0
dosqrts:$[dosqrt]
dosqrt2:. dosqrts
dosqrt2 2.0
dosqrt~dosqrt2

/ A bracket projection literal is already a function VALUE.  Its closing
/ bracket is therefore the same left boundary before a dyad as assigning the
/ projection to a name; it must not become a parse-time train constituent.
(+[;2],)3
bpcomma:+[;2],
bpcomma
bpcomma 3 1 2
bpcommas:$[bpcomma]
bpcommas
bpcomma2:. bpcommas
bpcomma2
bpcomma2 3 1 2
bpcomma~bpcomma2
bprcomma:+[2;],
bprcomma
bprcomma 3 1 2
bpadd:+[;2]+
bpadd
@[bpadd;3 1 2;:]
bpname:+[;2]
bpnadd:bpname+
bpnadd
bpadd~bpnadd
bpmatch:+[;2]~
bpmatch
bpmatch +[;2]
bpat:+[;2]@
bpat
bpat 3 1 2

\\
