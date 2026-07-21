/ amend never mutates a shared object: every in-place write in the kamend*
/ family is gated by k->r (entry CoW + amend_privatize in the descent).
/ A shared child reached by a path amend is copied at its level; the other
/ holders keep the old value.

/ cyclic take: rows 0 and 1 are the SAME object, b aliases it too
c:2#,!10
b:c 0
.[`c;0 2;:;99]
c
b

/ symbol-form nested path amend, child shared with a variable
C:(1 2 3;4 5 6)
b2:C 0
.[`C;0 2;:;99]
C
b2

/ value bracket form, two subscripts
D:(1 2 3;4 5 6)
b3:D 0
D[0;2]:77
D
b3

/ flat amend replaces the slot; a holder of the old row keeps it
L:(1 2;3 4)
m:L 1
L[0]:7 7
L
m

/ flat amend on a shared spine copies the spine
x:1 2 3
y:x
x[0]:88
x
y

/ value dot-amend leaves the source intact
E:(1 2 3;4 5 6)
.[E;0 2;:;55]
E

/ value at-amend leaves the source intact
t:(1 2;3 4)
@[t;0;:;5 5]
t

/ dict: path amend into a shared value vector copies it
d:.((`a;1 2 3);(`b;4 5 6))
e:d`a
.[`d;(`a;1);:;9]
d`a
e

/ compound bracket amend, shared child unharmed
G:(10 20;30 40)
h:G 0
G[0;1]+:5
G
h

/ 3-arg dot amend (monadic f, kamend3_ walker): same rule
c3:2#,!10
b4:c3 0
.[`c3;0 2;{99}]
c3
b4

/ 3-arg value form leaves the source intact
F:(1 2 3;4 5 6)
.[F;0 2;{55}]
F
\\
