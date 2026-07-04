/ cond/do/while/if in operand position: the call node reaches r44, which
/ must route to the lazy statement-position evaluators instead of eagerly
/ reducing the bracket group and applying the verb (was: valence error).
$[1;`t;`f],"asdf"
"asdf",$[1;`t;`f]
$[1;2;3]*$[1;4;5]
x:$[1;2 3;4],5
x
$[1;"abc";"d"]@0
c:0
$[1;2;c::99],5
c
do[2;c::c+1],7
c
while[c<5;c::c+1],7
c
if[c>2;c::0],7
c
/ error-class propagation: an error raised while evaluating a condition or
/ count must surface as itself, not be masked into 'type by the int check.
@[{$[zzz1;1;2]};0;:]
@[{do[zzz2;1]};0;:]
@[{while[zzz3;1]};0;:]
@[{if[zzz4;1]};0;:]
\\
