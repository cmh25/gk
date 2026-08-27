/ Fused *|x: exact first(reverse(x)) semantics without the reverse temporary.
/ Cover every flat vector type, a boxed list, and the empty prototypes.
*|1 2 3
*|1 2 3j
*|1.0 2.0 3.0
*|1.0e 2.0e 3.0e
*|"abc"
*|`a`b`c
*|(1;2j;"c";`d)

(*|!0)~0
(*|0#0j)~0j
(*|0#0.0)~0.0
(*|0#0.0e)~0.0e
(*|"")~" "
(*|0#`a)~`
(*|())~nul

/ Atoms and subtypes pass through both reverse and first unchanged.
*|7
*|2.5
d:.+(`a`b;1 2)
(*|d)~d

/ Closed/composed spellings remain ordinary function values and agree with
/ an explicit parenthesized pipeline.
f:*|
f 4 5 6
(*|)7 8 9
*(|10 11 12)
\\
