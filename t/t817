/ a fixed dyad (keeper) such as (1+) is the same monadic callable as the
/ projection +[1;], and takes its paths: adverbed and held as a value
/ (h:(1+)'; h 1 2 3, h@x, h'(1 2;3 4)), with bracket arguments ((1+)'[1 2 3],
/ (0&)/[5]), and as the function of a do/while controller (3 (1+)/0,
/ {x<5}(1+)/0, 2 k\0, 3 h/0).  fe's wrapper peel had no keeper arm, p.c's
/ juxtaposition case rejected the inner, and the keeper's own apply case
/ dropped the controller so these type-errored or converged forever.
/ Reference K agrees with every value below.
h:(1+)'
h 1 2 3
h[1 2 3]
h@1 2 3
h . ,1 2 3
h'(1 2;3 4)
(1+)'[1 2 3]
(1+)'1 2 3
q:(1+)
q'[1 2 3]
(0&)/[5]
(0&)\[5]
w:(0&)/
w 5
w[5]
3 (1+)/0
3 q/0
{x<5}(1+)/0
2 q\0
3 h/0
3 (1+)'/0
3 +[1;]'/0
2 +[1;]'\0
3 (1,)/0
(2#)'[1 2]
f:{(x,)'[("a";"b")]}
f"1"
f"2"
/ a bare primitive with brackets inside a postfix chain is a bracket call:
/ m:+[1;]' used to look m up (value error) because + was left a dyad
m:+[1;]'
m 1 2 3
m2:+[;1]'
m2 1 2 3
(m3:+[1;]') 1 2 3
/ each over a projection iterates its NEW arguments only: a parenthesized
/ or bracket-each value used to zip the fixed argument too (1 4 7, and a
/ rank error for bin), unlike the inline spelling
(+[1 2 3;]')0 2 4
+[1 2 3;]'0 2 4
f2:+[1 2 3;]
f2'[0 2 4]
({x+y}[1 2 3;]')0 2 4
(bin[1 2 3]')0 2 4
bin[1 2 3]'0 2 4
(+[1;]')5
{x+y}'[1 2 3;4 5 6]
\e 0
@[{(1+)'[1 2 3;4 5 6]};0;:]
\\
