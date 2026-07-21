/ two parse bugs around a bare name and a nested lambda.
/
/ 1. a monadic builtin is a do/while CONTROLLER, like every other spelling of
/    one.  ref.md: "do/while read the left argument to pick the mode when f is
/    monadic: a non-negative int is do, a monadic predicate is while.  The infix
/    and bracket spellings are the same (a f/x is f/[a;x])."  A bare builtin
/    parsed as the OUTER MONAD instead, so the derived verb reduced as a plain
/    monadic f\x -- converge -- and `prime{x+1}\4` spun forever, silently.
/ 2. an identifier immediately followed by `{` broke the implicit-x/y/z scan
/    (fn.c missed an n++ entering the nested-lambda state, so the depth went
/    negative and the scan never came back).  Every x/y/z AFTER such a nested
/    lambda was invisible: the params were never bound (a reference reached past
/    the fn for a global that wasn't there) and the valence was UNDER-COUNTED
/    (`val{g{1};x+y}` said 1, i.e. monadic, for a dyad; it never said 0 -- val
/    floors at 1, gk has no niladic fn).  `{f{1};x}` value-errored, but
/    `{f {1};x}` -- one space -- was fine.

/ --- 1. builtin as do/while controller.  prime 4 = 0, so it stops at once ---
prime{x+1}\4                 / -> ,4   was an infinite loop
{x+1}\[prime;4]              / bracket spelling
{prime x}{x+1}\4             / lambda controller
{c:prime;c{x+1}\4}[]         / the same builtin, via a name
(prime){x+1}\4               / parenthesized
in[;1 2 3]{x+1}\1            / -> 1 2 3 4   val-1 projection controller
prime{x+1}/4                 / -> 4   while-over
{x+1}/[prime;4]
{prime x}{x+1}/4
/ it must survive an operand parked to its left (assignment, a dyad's left arg)
{a:prime{x+1}\4;a}[]         / -> ,4
(,4)~prime{x+1}\4            / -> 1    in a match's right slot
#prime{x+1}\4                / -> 1    under a monadic count
/ and inside a lambda, with an implicit param (needs BOTH fixes)
{prime{x+1}\x}4              / -> ,4
{[q]prime{x+1}\q}4           / -> ,4
/ ONLY a builtin MONAD is a controller.  A dyad-capable head still applies to
/ the result: it takes a left value as its LEFT ARGUMENT, never as a loop count
/ (doc/digraphs.md).
#{z;x+y}\[0;1 2 3;nul]       / -> 4
|/{z;x+y}\[0;1 2 3;nul]      / -> 6
-{z;x+y}\[0;1 2 3;nul]       / -> 0 -1 -3 -6   the head negates the scan
/ each is not a controller form, so a builtin head still composes there
prime{x+1}'1 2 3             / -> 1 1 0
/ do-n (int controller) untouched
3{x+1}\1                     / -> 1 2 3 4
{x+1}\[3;1]
2 sqr/2.0                    / -> 16.0   builtin as the ITERATED verb (exact)
/ plain builtin application untouched
sqr 4                        / -> 16.0
sqr 1 2 3                    / -> 1 4 9.0
sqr[1 2 3]
prime'1 2 3 4                / -> 0 1 1 0
/ a bare verb is not a noun, so `@` after one is monadic (is-atom) and composes:
/ sqr@1 2 3 is sqr(@1 2 3) = sqr 0.
sqr@1 2 3                    / -> 0.0
{f:sqr;f@1 2 3}[]            / -> 1 4 9.0   but a VALUE named f indexes

/ --- 2. implicit x/y/z after `name{...}` ---
g:{y}
{g{1};x}4                    / -> 4   was a value error
{g{1};y}[4;5]                / -> 5
{g{1};z}[4;5;6]              / -> 6
{g{1};x+y+z}[1;2;3]          / -> 6
val{g{1};x}                  / -> 1   floors at 1: every fn takes >=1 arg
val{g{1};x+y}                / -> 2   was 1 -- under-counted, claimed monadic
val{g{1};x+y+z}              / -> 3   was 1
/ the spellings that already worked must keep working
{g {1};x}4                   / a space ended the identifier
{(g){1};x}4
{g[{1}];x}4
{x;g{1};x}4
/ nesting deeper than one level
{g{{1}};x}4
{g{1};g{2};x}4
/ formal params still suppress the implicit scan
{[a;b]g{1};a+b}[3;4]         / -> 7
val{[a;b]g{1};a+b}           / -> 2
\\
