/ .r -- the reserved-name namespace.  Every kreserved() name is a key (!.r
/ lists the reserved words) and each value is what the lexer emits for the
/ name, so .r entries print and APPLY like the builtins they name --
/ including the k.core predefined fns (0xc9/0xca/0xcb: gtime dv ssr ...),
/ which as VALUES resolve through fnpd in fe()/pgreduce, and copy (kcpcb)
/ and hash (khashcb) like any other verb value.
!.r                            / every reserved word, in RTAB order
#!.r
&/(`nul`sin`bin`binl`do`dv`ssr)lin !.r     / all present -> 1

/ nul, and the int-index builtins (0xc6/0xc7): juxt, bracket, infix
.r.nul
.r.sin 0
.r.sin[0]
#.r.draw[3;10]
2 .r.in 1 2 3
.r.gcd[12;18]

/ the predefined k.core fns (0xc9/0xca/0xcb) out of .r
.r.gtime 100
.r.gtime[100]
.r.dv[1 2 3;2]
1 2 3 .r.dv 2
.r.ssr["abc";"b";"x"]

/ fetched into a variable, then applied
g:.r.dv
g[1 2 3;2]

/ bd/db round-trip of a .r value
(db bd .r.dv)[1 2 3;2]

/ copy (kcpcb) and hash (khashcb) walk 0xc9-0xcb values without erroring:
/ amend copies the dict holding a 0xca value; distinct hashes the values
d:.,(`f;.r.dv;)
d2:.[d;`g;:;2]
d2[`f][1 2 3;2]
#?(.r.dv;.r.dv)                / equal verb values hash equal -> 1

/ reserved names cannot be assigned -- including bin/binl, which the old
/ hand-maintained kreserved() chain MISSED (bin:5 was silently swallowed:
/ no error, no assignment)
@[{bin::x};5;:]
@[{binl::x};5;:]
@[{sin::x};5;:]
/ .r itself is protected by the top-level single-letter guards
@[{.r.sin::x};5;:]
\\
