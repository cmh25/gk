/ force-monad: {Vx} canonical lambdas collapse to the monadic primitive
/ (fast path in fne_); must stay semantically identical to the spelled-out
/ {  Vx} lambda, print verbatim, and dispatch normally under adverbs.

/ basic monads
{,x}5
{,x}1 2 3
{-x}5
{|x}1 2 3
{#x}1 2 3 4
{<x}3 1 2
{%x}4
{~x}0 1 2
{!x}3

/ each / scan drive through the collapsed path
{,x}'1 2 3
{-x}'1 2 3
5{,x}\1

/ collapsed result == spelled-out (non-canonical) result
({,x}5)~({ ,x}5)
({-x}'1 2 3)~({ -x}'1 2 3)
({|x}1 2 3)~({ |x}1 2 3)

/ print is verbatim (no reconstruction); canonical vs spaced both echo source
{,x}
{ ,x}
{,x}~{,x}

/ interacts with the new numeric types (long / real)
{-x}5j
{,x}3j
{|x}1 2 3j
{-x}2.5e
{,x}1.0e 2.0e
{%x}4.0e
{|x}1.0e 2.0e 3.0e

/ projection / valence still behave (force monad is valence 1)
4:{,x}
{,x}[1 2 3]

/ direct bracket apply {Vx}[a] (fne_fast path) collapses too
{,x}[5]
{-x}[5]
{|x}[1 2 3]
{#x}[1 2 3 4]
{-x}[2.5e]
{,x}[3j]
({,x}[5])~({ ,x}[5])
({-x}[1 2 3])~({ -x}[1 2 3])

/ under an adverb, {Vx} dispatches identically to the bare primitive V
({,x}'1 2 3)~(,'1 2 3)
({-x}'1 2 3)~(-'1 2 3)
({,x}/1 2 3)~(,/1 2 3)
({-x}\1 2 3)~(-\1 2 3)
({|x}'(1 2;3 4))~(|'(1 2;3 4))
/ but the monadic-iterate seed/count forms must NOT collapse to dyadic:
/ they stay identical to the spelled-out lambda (which is monadic)
(5{,x}\1)~(5{ ,x}\1)
(5{,x}/1)~(5{ ,x}/1)
5{,x}\1
\\
