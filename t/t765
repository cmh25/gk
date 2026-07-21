/ two AFL hangs (2026-07-15), both quadratic/repeat-cost blowups on large
/ mixed-list inputs.
/ 1: amend with a MIXED index list recursed per element through kamendi4v,
/ and every recursive call ended in knorm -- a full rescan of the base once
/ a float value forced it mixed, O(n^2) (183k indices was a >20s hang; the
/ long-index-vector narrowing added earlier didn't cover mixed lists).
/ knorm now runs once, in a wrapper.  n kept modest here: the pre-fix cost
/ at 30k is ~1s, a visible hang-vs-instant diff without slowing the suite.
a:b:!30000
a[b,0j]:0!0ne
a[0]
a 29999
#a
/ semantics unchanged: mixed index, float value promotes cells, final
/ knorm still collapses a uniform result back to a vector
c:5 6 7
c[(0;2j),1]:1.5
c
b:!5
b[(1;3)]:9
4:b
@[2 3 4;(0;1j);:;9.5 8.5]
/ 2: factor of a general LIST went through irecur1 with no repeat cache
/ (the int/long vector paths have one), so 92k copies of one hard 63-bit
/ long each paid full Pollard rho.  the list path now reuses the same
/ 8-slot cache.
#factor 30000#19 45,9223372036854775806j
factor(12;9223372036854775806j;(4;,18j))
factor 3#(60;60j)
\\
