f:{x+y*z}
g:{x+y}

/ valences are recursive over the av string
val f
val g
val f'
val g'

/ each on conformable plist of args (full apply)
{x+y*z}'[2;3;4]
g'[1 2;3 4]
{x+y}'[1 2;10 20]

/ dyadic-juxt each: left atom over right vector
1 g'!5

/ partial-apply with each preserved via name binding
v:f[2]'[3]
val v
v 4
v[4]

/ partial-apply chained with parens (mid-chain ' wraps via fapply
/ av-lift; printed shape keeps the trailing each)
f[2]'[3]
(f[2]'[3])'[4]
((f[2])'[3])'[4]
(f[2]'[3])'(!3)

/ adverb on a slot-projection then partial-apply with each on the
/ remaining slots
f[;2;]'[3 4;5 6]
v3:f[;2;]'[3 4]
val v3
v3[5 6]
/ juxt and bracket apply must agree (no extra ' applied:
/ both consume the single remaining slot with stored eachparam).
v3 5 6
/ explicit ' on a projection with merged-in scalar slots
/ must iterate the newest (vec) arg, not the inner scalar slot.
v3'5 6
v3'[5 6]

/ each preserved on a 2-arg projection awaiting its second
v2:g'[1 2]
val v2
v2[3 4]

/ chained '[v]...'[v] gives Cartesian iteration over
/ each vec arg.  3-deep with vec args expands to 2x2x2; with atom
/ args collapses to a scalar.
{x+y*z}'[2]'[3]'[4]
{x+y*z}'[1 1]'[2 2]'[3 3]
{x+y}'[1 1]'[2 2]
{x+y}'[1 2]'[10 20]

/ single ' multi-arg is eachparam, not Cartesian.
{x+y*z}'[1 1;2 2;3 3]
{x+y}'[1 2;10 20]

/ adverb directly after a partial-args projection, no
/ parens (full composability).
f[1]'
f[1]'[2]
f[1]'[2]'
f[1]'[2]'[3]
f[1;2]'
f[1;2]' 3
{x+y*z}[1]'
{x+y*z}[1;2]' 3
val f[1]'
val f[1;2]'

\\
