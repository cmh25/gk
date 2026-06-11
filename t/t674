/ bracket spelling of the do/while adverb forms: f/[n;x] == n f/x etc.
/ when f is monadic, the 2-arg bracket form is do (n int) or while (b monadic),
/ matching the infix forms and the k reference.
f:{x+1}
5 f/0
f/[5;0]
5 f\0
f\[5;0]
{x<100}f/2
f/[{x<100};2]
{x<100}f\2
f\[{x<100};2]
/ controller must be int (do) or monad (while); other types -> type error
.[f/;(2.3;0);:]
.[f/;(`a;0);:]
.[f/;("z";0);:]
.[f/;({x+y};0);:]
.[f\;(2.3;0);:]
/ but wrong *arity* (3 args to a monadic f) is a valence error, not type
.[f/;1 2 3;:]
.[f\;1 2 3;:]
/ seeded over/scan: arg0 = accumulator, the rest conform (lists iterate in
/ lockstep, atoms broadcast as constants).  works for any valence >= 3.
g3:{[a;b;c]a+b+c}
g3/[0;1 2 3;10]
g3\[0;1 2 3;10]
g4:{[a;b;c;d]a+b+c+d}
g4/[0;1 2 3;10;100]
g4\[0;1 2 3;10;100]
g5:{[a;b;c;d;e]a+b+c+d+e}
g5/[0;1 2 3;4 5 6;7 8 9;10 11 12]
g5\[0;1 2 3;4 5 6;7 8 9;10 11 12]
/ an elided slot projects: f/[5;] is {x f/y}[5;], a do/while waiting on the
/ missing arg. it prints as {x+1}/[5;].
f/[5;]
pf:f/[5;]
pf 0
pf 3
pc:f/[;0]
pc 5
pw:{x*2}/[{x<100};]
pw 1
pw 5
ps:f\[5;]
ps 0
(f/[5;]) 0

{,x}\[5;][0]
{,x}\[5;][0][1]
{,x}\[5;][0]1
{,x}\[5;][0]@1
{,x}\[5;][0] . 1

f:{,x}\[5;]
f
f[0]
f[0]1
f[0][1]
f[0]@1
f[0] . 1

\\
