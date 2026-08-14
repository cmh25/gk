/ 2026-07-13 dot with an EMPTY right argument: two rulings.
/ DATA: `.` takes a PATH, and an empty path of ANY type is the value
/ itself: d . i is d@/i, and a fold over zero indices
/ returns the seed).  a . !0 used to convert the
/ typed empty to its prototype atom and return a@0 -- silently the FIRST
/ ELEMENT ("ab" . !0 was "a"); !0j accidentally already did the right
/ thing, () always did.
1 2 . !0
1 2 . !0j
1 2 . 0#0.0
1 2 . ""
1 2 . 0#`
1 2 . ()
"ab" . !0
5 . !0
5 . ()
(1 2;3 4) . !0
d:.((`a;7);(`b;8))
d . !0
d . 0#`
d[]
v3:9 8 7
`v3 . !0

/ Nonempty data paths: long/real vectors use the same deep-index dispatch as
/ the older flat vector types. These two used to rank-error.
1 2j . 0
1.0e 2.0e . 0

/ A typed vector on the right of function `.` is an argument vector, for every
/ callable subtype. Long/real used to be passed as one argument instead.
{x+y} . 1 2j
{x+y} . 1.0 2.0e
.[+;1 2j]
.[+;1.0 2.0e]
mul . 2 3j
mul . 2.0 3.0e
p:{x+y}[10;]
p . ,2j
p . ,2.0e
q:+'
q . 1 2j
q . 1.0 2.0e
fd:5+
fd . ,2j
fd . ,2.0e
co:-(2+)
co . ,3j
co . ,3.0e
/ FUNCTIONS: `.` takes an ARGUMENT LIST, and a typed empty still calls
/ with that type's PROTOTYPE argument (t647 "prototype conformance",
/ a . x = a[*x]); the long/real arms were missing -- f . !0j passed the
/ empty vector itself as the argument.
f:{1,x}
f . 0#0
f . 0#0.0
f . ""
f . 0#`
f . !0j
f . 0#2.0e
f . ()
\\
