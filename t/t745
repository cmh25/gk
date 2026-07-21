/ dict hash index (dict.c slot 3) -- lookup/update is O(1) past 32 keys via a
/ side open-addressing table over the interned key pointers, built lazily.
/ Semantics must be IDENTICAL to the old linear scan: insertion order,
/ duplicate-key FIRST-WINS, print/unmake/flip/bd shapes, and the wire format
/ (the table is never shipped -- db re-adds the slot).  Also covers the
/ nul-amend iteration-bound fix: a mid-level nul used to terminate on the
/ OUTERMOST target's length, silently skipping the tail (inner longer) or
/ index-erroring (inner shorter).

/ --- a big (indexed) dict from key/value vectors
kk:`$"k",'$!2000
dd:.+(kk;!2000)
dd[`k0]                        / first key
dd[`k1999]                     / last key
nul~dd[`nosuch]                / a miss -> 1
#!dd
/ update through the index; then append past the build
dd[`k777]:-777
dd[`k777]
i:0
while[i<500;@[`dd;`$"n",$i;:;i];i:i+1]
#!dd                           / -> 2500
dd[`n499]
dd[`k777]                      / the pre-append entry still resolves

/ --- duplicate keys keep FIRST-WINS through the index
du:.+((`$"d",'$!40),`d5;(!40),999)
#!du                           / 41 -- BOTH `d5 entries are physically present
du[`d5]                        / -> 5, the FIRST one
du[`d5]:-5
du[`d5]                        / the write hit the first one too
(. du)[40;1]                   / -> 999, the shadowed duplicate is untouched

/ --- structural views hide the index slot
#*. du                         / an unmade entry is still a TRIPLE
(5:du)~5:db bd du              / repr survives the wire -> 1

/ --- wire format unchanged: indexed and never-indexed dicts serialize alike
sd:.+(`a`b;1 2)
(db bd sd)~sd                  / small (never indexed) -> 1
dd2:db bd dd
dd2~dd                         / big (indexed) -> 1
#!dd2
dd2[`k1234]                    / and the rebuilt table works on the far side

/ --- equal dicts hash equal regardless of index state (khashcb keys+vals)
#?(sd;.+(`a`b;1 2))            / -> 1

/ --- copy of an indexed dict (dcp) stays correct
dc:.[dd;`zz;:;42]
dc[`zz]
dc[`k777]
#!dd                           / the source is untouched -> 2500

/ --- nul amend at depth, UNEQUAL inner/outer lengths (the k.c bug this
/ exposed: the bound came from the outermost target, not the frame's list)
.[((1 2;3 4;5 6);(7 8;9 10));(0;nul;0);:;99]   / inner LONGER: row 3 was skipped
.[((1 2;3 4);(7 8;9 10);(11 12;13 14));(0;nul;0);:;99]  / inner SHORTER: 'index
/ nul over a dict's values at depth (the t521 shape; dicts are 4-slot now)
ff.a:1
ff.b:2
gg.a:3#ff
.[gg;(`a;nul;`a);-;1]
\\
