/ A symbol handle indexed with brackets derefs the named global and indexes it.
a:10 20 30
`a[0]                                 / 10
`a[2]                                 / 30
`a[0 2]                               / 10 30
`a 1                                  / 20  -- juxtaposition still derefs
`a@1                                  / 20  -- @ still derefs
`a . 1                                / 20  -- . still derefs
m:(1 2 3;4 5 6)
`m[1]                                 / 4 5 6
`m[1;2]                               / 6
`m[0;0]                               / 1
`m[1][2]                              / 6   -- chained bracket group
h:`a
h[1]                                  / 20  -- a local holding the handle
f:{x[1]}
f`a                                   / 20  -- lambda given a handle (the motivating case)
g:{x[y;z]}
g[`m;1;2]                             / 6   -- handle deep-indexed inside a lambda
/ writes through the handle still amend the global in place
.[`a;1;:;99];
a                                     / 10 99 30
@[`a;2;+;1];
a                                     / 10 99 31
/ an UNDEFINED handle resolves to nul. nul is the identity
/ VECTOR, not the identity function: an integer index returns itself, a symbol
/ index misses (-> nul), an empty index gives (), and nothing else is a valid
/ index at all.  A non-nul atom cannot be indexed by anything, not even nul.
`undefh 1
`undefh[1]
(`undefh 1)~`undefh[1]
`undefh[1 2]
1~nul 1
1~nul[1]
(1 2)~nul[1 2]
(1 2 3)~nul 1 2 3
(-1)~nul -1
1j~nul 1j
/ symbol index misses; empty index gives ()
nul~nul `b
()~nul ()
()~nul ""
()~nul nul
()~nul[]
()~nul[nul]
/ a char, a float or a dict is not an index at all
@[.;"nul \"b\"";:]
@[.;"nul 1.5";:]
@[.;"nul 1.5e";:]
d:.+(`a`b`c;1 2 3)
@[.;"nul d";:]
/ a non-nul atom ranks on any index, nul and () included
@[.;"5 nul";:]
@[.;"5 ()";:]
@[.;"5[nul]";:]
@[.;"1[2]";:]
@[.;"\"b\" nul";:]
/ containers elide: x nul is all of x's elements
(1 2 3)~(1 2 3) nul
"ab"~"ab" nul
()~() nul
(1 2 3)~d nul
/ a path applies one level at a time: nul . (1;2) is (nul[1])[2] is 1[2]
1~nul . ,1
@[.;"nul[1;2]";:]
@[.;"`undefh[1;2]";:]
\\
