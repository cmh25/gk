/ Two AFL findings (2026-07-15, round 7), both in the FUZZING build only but
/ rooted in real code paths; this test pins the production-visible semantics.
/
/ 1: factor of a long vector kept an 8-slot repeat cache keyed by v&7 -- the
/ low 3 bits only.  2 and INT64_MIN+2 (= -9223372036854775806) share those
/ bits, so a vector alternating them collided every element into one slot and
/ paid a fresh Pollard-rho factorization per element (a fuzz hang; under the
/ fuzz budget the same input also 'limit'd rather than running for minutes).
/ A Fibonacci hash spreads all input bits, so both values cache.  Results are
/ unchanged -- the cache is a speedup, not a semantic.
factor 2 -9223372036854775806j
#factor 8000#2 -9223372036854775806j
factor 3#2 -9223372036854775806j
/ 2: group (=) of a general list ran a count pass then a fill pass, sizing
/ each per-slot result vector by the count.  Both derive a slot from khash,
/ which under FUZZING charged a *global* per-node budget; spent across the many
/ khash calls of one group, it crossed 0 mid-operation, so equal rows hashed to
/ their real slot before exhaustion and to KERR_STACK's slot after -- the
/ passes disagreed and the fill pass wrote past a count-pass-sized vector (heap
/ overflow).  khash's budget is now reset per top-level call, so it is a pure
/ function of its argument again and equal rows hash alike.  A normal build has
/ no budget, so grouping identical rows always yields one group.
=3 2#,,(),,,,()
#=8000 3#,,(),,,,()
=`a`b`a`c
\\
