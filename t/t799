/ composition valence (v5.2.1, cde6f0c): val of a composition follows its
/ RIGHTMOST member (the one that receives the arguments); members before it
/ apply monadically.  Was hardcoded val=2, breaking monadic juxtaposition.
a:{x}1-
val a
/ verb-headed composition: still val 1 (verb applies monadically to {x}1-x)
b:-{x}1-
val b
/ rightmost member is a dyad-capable primitive -> val 2
c:+{x}*%
val c
d:&+{x}*%
val d
/ builtin monad rightmost -> val 1
e:,sqrt
val e
/ postfix adverbs on the composition: each preserves valence, over collapses
f:(,{x}1-)'
val f
g:(,{x}1-)/
val g
\\
