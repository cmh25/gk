/ amend of a WIDER-typed value VECTOR (long -8 / real -9) into a narrower numeric base
@[8#0; 1 2; :; 2 3j]           / long-vec into int base   -> (0;2j;3j;0;0;0;0;0)
@[8#0; 1 2; :; 2 3.0e]         / real-vec into int base
@[8#0.0; 1 2; :; 2 3j]         / long-vec into float base
@[8#0; 1 2; +; 2 3j]           / with a dyad, not :
v:8#0; @[`v;1 2;:;7 8j]; v     / via symbol handle (pass-by-name)
w:8#0; w[1 2]:7 8j; w          / v[idx]:vals form

@[8#0; 1 2; :; 5j]             / scalar long broadcasts (already worked)
@[8#0; 1 2; :; 2.5 3.5]        / f64 vector promotes (already worked)
@[8#0j; 1 2; :; 2 3j]          / matching long base stays a long vector

a:(1 2;(3 4;5 6))
.[a;(1;1;0 1);:;-9j]           / deep scalar long (always worked)  -> (1 2;(3 4;-9 -9j))
.[a;(1;1;0 1);:;-8 -9j]        / deep long-vector (was broken)     -> (1 2;(3 4;-8 -9j))
.[a;(1;1;0 1);:;2.0e 3.0e]     / deep real-vector
\\
