/ Local memory-safety fixes from the 2026-07-25 audit (batch 2).
/ Errors are trapped with @[f;x;:] so one file covers several cases.

/ 1. la.c: a matrix row that is itself NESTED is not flattened by `1.0*row`,
/ so the reader reinterpreted the row's K words -- heap POINTERS -- as ints
/ via fi().  det ((1 2;3);(4;5)) returned a DIFFERENT number every run: live
/ ASLR'd addresses leaking into a user-visible result.  Must be a clean error.
@[det;((1 2;3);(4;5));:]
@[rref;((1 2;3);(4;5));:]
@[lu;((1 2;3);(4;5));:]
/ legitimate matrices are unaffected
det (1 2.0;3 4.0)
det (1 2;3 4)
det ((1;2);(3;4))
inv (2 0.0;0 2.0)

/ 2. la.c mag_: a nested row makes the recursive call return a VECTOR, and
/ fk() on a vector reads the ko union's data POINTER as a double.
@[mag;((1.0 2.0;3.0 4.0);5.0 6.0);:]
mag 3 4.0

/ 3. `1:` mmap was the only producer of a NON-NUL-terminated -3 char vector,
/ but fopen_/del_/rename_/3:/4:/8: all use px(x) as a C string.  At
/ 24+len == one page exactly, strlen ran off the mapping and spliced adjacent
/ heap bytes into the error text.  Check the name ENDS where it should (the
/ OS message itself differs per platform, so only the boundary is asserted).
/ 4072 is deliberate: 24+4072 is a multiple of 4096 but NOT of 65536, which is
/ what Cygwin/MSYS report as the page size -- this line is the regression test
/ for that too.
("z780")1: 4072#"a"
p:1:"z780"
(4072#"a")~p
e:@[{0:x};p;:]
(4072#"a")~4072#e 1
":"~(e 1)@4072
del"z780"

/ 4. `N $ x` truncation: i32 strlen went negative past 2^31 and turned the pad
/ branch into a heap overflow.  Small cases must be unchanged.
5$"abc"
-5$"abc"
2$"abcde"
-2$"abcde"
3$"abc"
\\
