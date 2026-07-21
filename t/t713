/ Projecting an adverbed BUILTIN must preserve the adverb.
in'[1]                          / in'[1]
ss'[1]                          / ss'[1]
in'["."]                        / in'["."]

"." in' ("a.b";"cd")            / 1 0   -- direct, for reference
(in'["."]) ("a.b";"cd")         / 1 0   -- projected must match
(in'["."]) ("1.5";"20";"yy")    / 1 0 0
f:in'["."]                      / bind left once, apply later
f ("x.y";"ab";"q.r")            / 1 0 1
(in'["a"]) ("cat";"dog";"bat") / 1 0 1  -- char "a" broadcast

/ JUXTAPOSITION bind-left: a noun left of an adverbed builtin projects too.
("."in') ("a.b";"cd")           / 1 0
("."in') ("1.5";"20";"z.z")     / 1 0 1
g:"."in'                        / bind via juxtaposition, apply later
g ("x.y";"ab";"q.r")            / 1 0 1
/ lambda projection is unchanged (control):
{x+y}'[1 2 3]                   / {x+y}'[1 2 3]
({x+y}'[1 2 3]) 10 20 30        / 11 22 33
\\
