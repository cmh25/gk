/ A PRIMITIVE verb applied to a TRAIN (a fixed-dyad/projection/composition
/ value: 0xd0/0xd9/0xc5) COMPOSES (prepends) instead of executing.
(#"."in') ("a.b";"cd";"e.e")   / 3     -- #("." in' x) = #(1 0 1)
(- "."in') ("a.b";".x")        / -1 -1 -- negate composed
(| "."in') ("a.b";"cd")        / 0 1   -- reverse composed
(# - "."in') ("a.b";"cd";"e.e") / 3    -- chained primitives compose
(|/ - "."in') ("a.b";"cd")     / 0     -- derived + primitive + train
($"."in') ("a.b";"cd")         / (,"1";,"0") -- format COMPOSES too: $("." in' x)=$(1 0)
$["."in']                      / "\".\"in'" -- bracket EXECUTES: source string
/ application path is unchanged (in' stays a runtime projection):
"." in' ("a.b";"cd")           / 1 0
ic'("ab";"cd")                 / (97 98;99 100)
in'[1]                         / in'[1]
("."in') ("a.b";"cd")          / 1 0
(|/"."in')'(("a.b";"ab");("1.5";"20"))  / 1 1
/ plain execution unaffected (operand is data, not a train):
#("a";"bb";"ccc")              / 3
#3 4 5                         / 3
\\
