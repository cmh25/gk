/ AFL+ASan bughunt 2026-07-14 (round 4): five fixes surfaced by an ASan
/ sweep of the fuzz queue.  Each line below is value-observable; the crash/
/ leak fixes that only manifest on an error path are exercised through the
/ @[f;x;:] / .[f;x;:] error trap so a value prints instead of halting.

/ 1. gcd/lcm on i64 MIN (0Nj read as its magnitude 2^63) used to negate
/    INT64_MIN in i64 -- UB that trapped (SIGILL) at -O3, or returned a
/    wrapped-negative gcd.  Now unsigned-core with a J_INF (0Ij) saturation.
gcd[0Nj;6j]
gcd[0Nj;0]
lcm[0Nj;2]
gcd[0Nj;0Nj]
gcd[-6;4]
lcm[-6;4]
gcd[-6j;4j]
gcd[-19790;29140]
lcm[-19790j;29140j]
gcd[96571774612149j;-98670646687123j]

/ 2. a boxed i64 atom applied with brackets (5j[]) ranked out but leaked the
/    boxed atom -- the rank-out path freed only f64 boxes, not i64.  Trapped.
@[{5j@x};0;:]

/ 3. deep amend with an out-of-range LONG index (.[m;i j;:;v]) leaked the
/    boxed index atom on the index-error exit of kamend3_/kamend4_.  Trapped.
@[{.[(1 2;3 4);7 0j;:;9]};0;:]

/ 4. db (deserialize) tracked the remaining byte count only through the
/    element headers, never the fixed scalar/vector PAYLOADS, so a truncated
/    or hostile blob kept passing the length guards while the read pointer ran
/    past the buffer -- a heap over-read.  n now decrements by every consumed
/    byte, and a mid-loop symbol-vector truncation frees its partial result.
(`a`s`d)~db bd `a`s`d
(.,(`a;7))~db bd .,(`a;7)
((1 2;`a`b;"xy"))~db bd(1 2;`a`b;"xy")
@[db;-2_bd .,(`a;7);:]
@[db;-2_bd`a`s`d;:]

/ 5. timer with a finite but astronomical interval (timer 9e95) overflowed
/    the double->i64 cast computing the next tick -- UB/SIGILL.  Now clamped.
$[nul~timer(1e99;{x});`ok;`bad]
timer 0;`done
\\
