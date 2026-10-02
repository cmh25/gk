/ a parenthesized bracket call or group in front of a juxtaposition chain.
/ r003_'s fallback wrote the complete call into the chain's occupied left
/ slot, discarding its head: (#[e]){y}5 lost the lambda and (#[e])1+2 gave 5.
/ A group never took the brackets that followed it, so (1+)[e]{y}5 was 1+5.
/ The projection-outer branch boxed a postfix adverb chain, stranding the
/ adverb: (#[;2])'p was a parse error.  $[x;] reached k() with a bare hole.
\e 0
e:1 2 3
p:10 20 30 40 50
x:1 2 3
(#[e]){y}5
#[e]{y}5
(x){y}5
@[{(#[e])1+2};0;:]
@[{#[e] 1+2};0;:]
(1 2 3)[1]{y}5
(x)[1]{y}5
x[1]{y}5
(1+)[e]{y}5
(2#)[e]{y}5
(1+)[e]*2
@[{(1+)[e] 5};0;:]
(1;2)[0]+1
(x)[0]{y}5
(#[;2])'p
#[;2]'p
(#[e;])'p
(bin[e])'p
(+[;2])'[p]
$[e;]
$[;e]
$[e;] 5
(x:$[e;]) 5
$[1;2]
$[0;2]
$[1;2;3]
$[0;2;3]
$[0;2;0;3;4]
/ unchanged neighbours
1+2*3
(1+2)*3
-2*3
(-%)[1;] 4
(+[1;]*+10)~+[1;](*+10)
\\
