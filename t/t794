/ Bracket projections of syntactically-known verbs remain verbs in trains.
/ Cover builtin, primitive, and predefined heads, including grouping and
/ adjacent projections.
(val[draw[1]*+])~2
(val[(draw[1])*+])~2
(val[+[1;]*+])~2
(val[(+[1;])*+])~2
(val[ep[-]*+])~2
(val[(ep[-])*+])~2
(val[draw[1]+[1;]])~1

/ Once the following train has an argument, its first verb must not consume
/ the projection as a dyadic left noun.  Explicit grouping states the same
/ parse and makes a useful discriminator.
(+[1;]*+10)~+[1;](*+10)
(mul[2]*+10)~mul[2](*+10)
(ep[-]*+!5)~ep[-](*+!5)

/ Projection arguments inside train bytecode are evaluated in local scope.
f:{[n]val[draw[n]*+]}
f[1]~2

/ Original report: a predefined projection outside an applied three-member
/ train.  The ungrouped and explicitly grouped forms must agree.
a:ep[-]*+10{x[0]:+/x;x[1]+:3;x}\1 4
b:ep[-](*+10{x[0]:+/x;x[1]+:3;x}\1 4)
a~b

\\
