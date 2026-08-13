/ Cross-verb fusion consumers.  These are semantic regression tests: each
/ spelling reaches a fused parser path on supported shapes, while empties,
/ sentinels, ties, and unsupported/error shapes retain ordinary behavior.
a:0 1 2 3 4
#&a<2
*&a>2
+/a<2
&/a<5
|/a>3
#&~a<2
*&~a>2
+/~a<2
&/~a<2
|/~a<2
#&a<0 2 2 4 9

e:0#0
#&e<0
*&e<0
+/e<0
&/e<0
|/e<0
#&~e<0
*&~e<0
+/~e<0
&/~e<0
|/~e<0

/ Direct filters and filtered reductions, including general/empty payloads.
m:0 1 0 1 0
y:10 20 30 40 50
y@&m=1
y@&~m=1
"abcde"@&m=1
`a`b`c`d`e@&~m=1
(1;2j;"c";`d;5.0)@&m=1
(0#0)@&0#0
+/y@&m=1
&/y@&m=1
|/y@&m=1
+/y@&~m=1
&/y@&~m=1
|/y@&~m=1
+/(0#0)@&0#0
&/(0#0)@&0#0
|/(0#0)@&0#0

/ f32 payloads and comparisons take the dedicated filtered-reduction path.
yf:1.5e 2.5e 3.5e 4.5e
cf:2.0e 2.0e 4.0e 4.0e
+/yf@&yf<3.0e
&/yf@&yf<cf
|/yf@&~yf<cf
ef:0#0.0e
+/ef@&ef<0.0e
&/ef@&ef<0.0e
|/ef@&ef<0.0e

/ Sentinel-aware int-to-long widening scan.  The fold remains on the generic
/ path, but stays here to cover the shared sentinel and empty semantics.
+/0j+1 2 3
+\0j+1 2 3
+/0j+0#0
+\0j+0#0
+\0j+0N 0I -0I 1

/ Group counts preserve first-occurrence order and the general empty result.
#'=2 1 2 3 1 2
#'=0N 0I -0I 0N 0I 0 -0I
#'=0n 0n -0.0 0.0 1.0 0n
#'="abacab"
#'=`b`a`b`c`a`b
#'=(1;2j;1;2j;"a";"a")
#'=0#0

/ Direct stable sort.  Duplicate values and float ties retain grade order.
si:3 1 2 1 0N 0I -0I 2
si@<si
si@>si
sf:3.0 0n -0.0 0.0 -0i 0i 1.0 0n
sf@<sf
sf@>sf
sc:"cabac"
sc@<sc
sc@>sc
sy:`b`a`b`aa`a
sy@<sy
sy@>sy
sx:0#0
sx@<sx
sx@>sx

/ Remaining direct kernels cover f32 signed-zero ties, long sentinels, and
/ heterogeneous general lists.
sfe:3.0e 0ne -0.0e 0.0e -0ie 0ie 1.0e 0ne
sfe@<sfe
sfe@>sfe
sj:0Nj 0Ij -0Ij -9000000000000000000 9000000000000000000 0j 1j 0j
sj@<sj
sj@>sj
sg:(2j;"b";1;`a;2j;0.0e;"a";1)
sg@<sg
sg@>sg

/ Fusion lookahead must not steal gk's value-level train composition.  Named
/ projections/compositions make the intended runtime behavior unambiguous.
p:2+;q:<p;q 3 1 2
p:2+;q:>p;q 3 1 2
p:2+;q:#'=p;q 3 1 3 2 3
p:<2+;q:>p;q 3 1 2
p:+[;2];q:<p;q 3 1 2

/ Stable partial grade and grade-at.  k=0 is valid; k>N/negative/sentinel
/ deliberately fall through and therefore retain take's cycling/errors.
tg:4 1 3 1 2 0 4
0#<tg
1#<tg
3#<tg
3#>tg
3j#<tg
7#<tg
8#<tg
-2#<tg
tg@0#<tg
tg@3#<tg
tg@3#>tg
"abcdefg"@3#<tg
`a`b`c`d`e`f`g@3#>tg

/ Sizes below the partial-grade cutoff exercise the dense and sparse kernels
/ (the seven-item cases above intentionally exercise full-grade fallback).
tu:4 1 3 1 2 0 4 9 8 7 6 5 9 8 7 6 5 2 3 0
3#<tu
3#>tu
tu@3#<tu
tu@3#>tu
tw:17 -5 1000000000 3 -1000000000 42 6 8 9 10 11 12 13 14 15 16 18 19 20 21
3#>tw
tw@3#>tw

/ Non-int top-k comparators and grade-at results.
2#<sfe
2#>sfe
sfe@2#<sfe
sfe@2#>sfe
2#<sj
2#>sj
tc:"cabacxyz"
2#<tc
2#>tc
2#<sg
2#>sg

/ Decline paths must preserve the original errors.
@[{#&1 2<1 2 3};0;:]
/ Scalar-left bare where and terminal consumers must decline before n().
&0=1 0 1
#&0=1 0 1
*&0=1 0 1
+/0=1 0 1
&/0=1 0 1
|/0=1 0 1
10 20@&0 1 0=1
+/10 20@&0 1 0=1
/ Scalar-left comparisons must decline before reading a vector count.
10 20 30@&0=1 0 1
+/10 20 30@&0=1 0 1
&/10 20 30@&0=1 0 1
|/10 20 30@&0=1 0 1
/ Scalar payloads and scalar grade/group inputs retain generic errors.
@[{0@&0 1 0=1};0;:]
@[{+/0@&0 1 0=1};0;:]
@[{#'=1};0;:]
@[{1@<1};0;:]
@[{0@1#<0 1};0;:]
@[{3#<1};0;:]
@[{1 2@3#<4 3 2 1};0;:]
@[{1 2@1#<0};0;:]
\\
