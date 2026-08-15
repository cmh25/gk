/ val() on a deeply nested composition: stack error (stderr), not a segfault.
/ \e 0: the depth makes the error surface outside an @[;;:] trap's window.
/ 5: is a program's string, so a too-deep display is a stack error there;
/ the echo instead elides with "..." (human channel, lossy like \p) -- not
/ echoed here because the elision depth differs between prod and ASan builds
f:{x}({x*2}@)
do[60000;f:{x} f]
\e 0
val f
5:f
\e 1

/ below the cap, function displays still round-trip (depth < ASan maxr=100)
g:{x}({x*2}@)
do[40;g:{x} g]
(g 3)~(. 5:g) 3

/ dotcb: a bare verb dot-applied with 3/4 args is not silently dot-amend;
/ @ gets at-amend, . gets dot-amend, anything else is a valence error
f:%
@[{f . (1 2 3;0;{77})};0;:]
q:@
q . ((1 2;3 4);0 1;:;(9;8))
(q . ((1 2;3 4);0 1;:;(9;8)))~@[(1 2;3 4);0 1;:;(9;8)]
d:.
(d . ((1 2;3 4);0 1;:;9 8))~.[(1 2;3 4);0 1;:;9 8]

/ invalid index: amend tolerates as a no-op, value falls back to the RHS
/ (pre-5.2.2 contract) instead of raising from the post-write selection
a:!3
r:a[`x]:5
(r;a)

/ the selected-region return survives in value contexts
a:!5
r:a[1 2]:9
(r;a)
{x[0]:8;x} 1 2 3
g:{x[1 2]:0}
(g 4 5 6)~0 0

/ 0xd0 with a lambda-projection operand is built as the equivalent verb
/ projection, so its display round-trips (5.2.0 fusion printed ",[g;]",
/ 5.2.2 printed "{x+y}[2;]," which did not re-parse)
g:{x+y}[2;]
h:g,
h 3
5:h
h~. 5:h
h~,[g;]
q:(2+),
q~. 5:q

/ literal grouped-callable RHS in indexed assignment reduces like a name
c:,0
c[0]:(2+),
c[0] 3
c[0]:+[;2],
c[0] 3
c[0]:(+/)
c[0] 1 2 3

/ form: long/real width vectors, and float-width atom broadcast
(1 2j)$3 4
1.1 2.1e$3 4
8.1 6.1$2
8 6$2j

/ a val-1 composition follows the do/while rule, infix and bracket
q:(<2+)
5 q/1 2 3
q/[5;1 2 3]
5 q\1 2 3
q\[3;1 2 3]
p:*[2;]
p/[3;1]
p\[3;1]
{x<20}(2*1+)/1

/ sleep: finite values past the integer range are domain errors
@[{sleep x};3e38e;:]
@[{sleep x};1e30;:]

/ compositions survive the wire round-trip (adverb slot pinned empty)
q~db bd q

/ bare verb / builtin / adverbed-verb VALUES assign to indexed targets the
/ same way they assign to names (assign44 arms at the lookahead sites)
c:!3
c[0]:+
c[0][2;3]
c[0]:+/
c[0] 1 2 3
c[0]:+'
c[0][(1 2;3 4)]
c[0]:0:
c[0]:gtime
c[0]:5 sin/
c[0] 2.0
c[0]:sqrt 256.
c[0]
d:.()
d[`f]:+/
d[`f] 1 2 3
m:(1 2;3 4)
m[0;1]:+/
m[0;1] 2 3 4

/ chained assignment of bare verb values: r:c[0]:*/ and r:v:*/ bind every
/ target to the verb -- even when a name was already bound
c:!10
r:c[0]:*/
r 2 3 4
c[1 2 3]
v:1 2 3
r:v:*/
(r 2 3 4;v 2 3 4)
x:!3
y:z:x[1]:+/
(y 1 2;z 1 2;x[1] 1 2)
w:!3
s1:w[0]:5 sin/
s1 2.0
\\
