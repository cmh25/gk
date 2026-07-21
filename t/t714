/ Tacit composition of a monadic verb with a PROJECTION.
kp:(("a.b";"ab");("1.5";"20"))
(|/"."in')'kp                   / 1 1   -- |/(any "." in the column's strings)
(&/"."in')'kp                   / 0 0   -- &/(all have ".")
(|/ "."in') ("a.b";"cd")        / 1     -- inline: |/("." in' ("a.b";"cd"))=|/(1 0)
(&/ "."in') ("a.b";".x")        / 1     -- &/(1 1)
/ bound-value compositions (verb value + projection value)
f:"."in'
g:|/
(g f) ("a.b";"cd")              / 1     -- derived-verb value composed
h:#
(h f) ("a.b";"cd";"e.e")        / 3     -- plain-verb value: #("." in' ..)=#(1 0 1)
r:|
(r f) ("a.b";"cd";"e.e")        / 1 0 1 -- reverse composed (|(1 0 1))
/ regressions: existing composes + plain application unchanged
(|/ 1+') 3 4 5                  / 6
(*&) 0 0 5 0                    / 2     -- *(&0 0 5 0)=*(,2)
|/ 1 2 3                        / 3
\\
