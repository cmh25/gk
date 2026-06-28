/ Coverage (bucket A): amend family (k.c kamendi3v/kamendi4v/kamendi4d/kamend3_/
/ kamend4_). @ (index) and . (path) amend, 3-arg (monadic f) and 4-arg (dyadic f;y),
/ over vectors / nested lists / dicts, by value and by reference. Self-validating:
/ amend-all-indices with f == f each (monadic) / the dyad (4-arg); spot results via ~.
/ -- vector @ amend, monadic: amend-all == each (int, long, float)
(@[1 2 3 4 5;!5;{x*x}])~{x*x}'1 2 3 4 5
(@[1 2 3 4 5j;!5;{x*x}])~{x*x}'1 2 3 4 5j
(@[1.5 2.5 3.5;!3;{x+x}])~{x+x}'1.5 2.5 3.5
/ -- vector @ amend, dyadic: amend-all == dyad
(@[1 2 3 4 5;!5;+;10 20 30 40 50])~1 2 3 4 5+10 20 30 40 50
(@[1 2 3 4 5;!5;*;2])~1 2 3 4 5*2
/ -- vector @ amend, partial index (scalar & vector)
(@[1 2 3 4 5;2;{10*x}])~1 2 30 4 5
(@[1 2 3 4 5;1 3;+;100])~1 102 3 104 5
/ -- path . amend on nested list, 3-arg and 4-arg
(.[(1 2 3;4 5 6);(0;1);{10*x}])~(1 20 3;4 5 6)
(.[(1 2 3;4 5 6);(1;2);+;100])~(1 2 3;4 5 106)
/ -- dict @ amend by value, monadic and dyadic (dict via .+(keys;vals))
((@[.+(`a`b`c;1 2 3);`a`b`c;{10+x}])`a`b`c)~11 12 13
((@[.+(`a`b`c;1 2 3);`a`c;+;10 30])`a`c)~11 33
/ -- by-reference amend (vector + dict), live mutate
x:1 2 3 4 5; @[`x;1 3;+;100]; x~1 102 3 104 5
.[`e;`p;:;5]; @[`e;`p;+;10]; (e`p)=15
/ -- error arms: out-of-range index, bad path
*@[{@[1 2 3;9;{x}]};0;:]
*@[{.[(1 2;3 4);(0;9);{x}]};0;:]
\\
