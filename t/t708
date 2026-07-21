/ lin: hashed builtin (was the {x in\y} bootstrap) -- same semantics, O(#a+#x)
2 5 9 lin 1 2 3
2 5 lin 1 2j
2j 5j lin 1 2j
1.5 2.5 lin 2.5 3.5
1.5e 2.5e lin 2.5e 3.5e
"axz" lin "abc"
`a`q lin `a`b`c
(1 2;3 4;5) lin (1 2;(,5);7)
2 5 lin (1;2;`a)
(1;2.0;`a;"x";1 2) lin (2.0;`a;1 2)
2 lin 1 2 3
`q lin `a`b
"x" lin "xyz"
1 2 lin !0
0N 0I 5 lin 0N 7
/ NaN and -0.0 membership agrees with ~, distinct, group (find fixed too)
0n lin 0n 1.0
1.0 0n lin ,0n
(,0n)?0n
0ne lin ,0ne
-0.0 lin ,0.0
/ first-class value and projection
f:lin
f
f 1 3 2
(f 1 3 2) 1 2
/ right side must be a list
.[lin;(1 2;3);:]
/ x?/y is the batched find all of the above ride on: first index, type-strict
2 7 2 7?/7 2
1 2 3?/("ab";2;2.0;3)
(1 2;"ab";5)?/(("ab");5;1 2;`q)
1.5 0n 2.5?/0n 2.5 9.0
0.0 -0.0 7.5?/-0.0 0.5
`a`b`a?/`b`q
"hello"?/"lox"
0N 0I 5?/0N -0I 5
0j 8j?/0 8
/ dvl and dv ride x?/y too
1 2 3 2 4 dvl 2 4
1 2 3 2 4 dv 2
/ empty haystacks and empty probes, various types
(!0)?/1 2
(0#0j)?/,1j
()?/1 2
1 2 3?/!0
1 2 lin 0#0.0
(0#`a) lin `a`b
/ sentinels and infinities, both widths and both sides
0i 1.5 -0i?/-0i 0i 0n
0ne 0ie 1.5e?/0ie 0ne
0Nj 0Ij 7j?/0Ij 0Nj -0Ij
1.5 0n 2.5 dvl ,0n
0n 0i dv 0i
/ scalar find on NaN, both float widths (fixed with the batch)
(,0ne)?0ne
(0n 1.0)?0n
(,1.0)?0n
/ subtypes: dicts and lambdas as items, matched whole-value
D:.((`a;1);(`b;2))
D2:.((`a;1);(`b;2))
F2:.,`a,7
D~D2
D2 lin (D;5)
F2 lin (D;5)
E0:.()
E0 lin ,E0
L2:{x+1}
L2 lin (1;L2)
{x+9} lin (1;L2)
(D;L2;"ab")?/(L2;F2;"ab")
/ list probes into a typed haystack miss whole
(1 2;3) lin 1 2 3
/ atom-left ?/ is still roll (draw), untouched by the batched find
#'3?/5 5 5
&/,/(3?/5 5 5)<5
\\
