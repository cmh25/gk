/ Integer-vector enumerate: mixed-radix odometer, rightmost changes fastest.
!0 0
!1 1
!2 2
!3 2
!2 3
!,3
!,0

/ Coordinate rows preserve the radix vector's width.
4:'!2 3
4:'!2 3j
!2 3j

/ Empty dimensions and zero radices.
!()
!!0
!0#0
!!0j
!0#0j
!2 0 3
!1000000000000000000 0j

/ General lists recurse.  Mixed int/long atoms remain a true general list;
/ whitespace vector notation promotes to a long radix vector instead.
!(2;3j)
(!(0 1;2 3 4))~(!0 1;!2 3 4)
!(nul;2)

/ Validate every dimension before zero short-circuiting.  All integer
/ sentinels are domain; floats are not radix vectors; product overflow is
/ wsfull and is detected before allocation.
@[!;0 -1;:]
@[!;0 0I;:]
@[!;2 0N;:]
@[!;2 -0I;:]
@[!;2 -1j;:]
@[!;2 0Nj;:]
@[!;2 0Ij;:]
@[!;2 -0Ij;:]
@[!;2 3.0;:]
@[!;1000000000 1000000000;:]

\\
