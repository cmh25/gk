/ a fixed-dyad keeper such as (x,) inside a lambda body is resolved from the
/ CALLER's scope on every call.  rd0 used to write the looked-up seed back
/ into the bytecode token that every call shares, so the first call's value
/ was baked in: f:{(x,)"a"}; f"1"; f"2" gave "1a" twice (v5.0.6 through
/ v5.3.0; reported by bleedsa as a path prefix stuck on the first directory).
/ rc5 did the same to the members of a composition.
f:{(x,)"a"}
f"1"
f"2"
g:{(x,)'("a";"b")}
g"1"
g"2"
h:{(x+)1}
h 1
h 2
l:{d:x;(d,)"a"}
l"1"
l"2"
c:{(|x,)"a"}
c"1"
c"2"
m:{[a;b](a,)b}
m["1";"x"]
m["2";"x"]
n:{(x,)}
(n"1")"a"
(n"2")"a"
{(x,)"a"}'("1";"2")
p:{[e;q](q,)'("a";"b")}
r:p"x"
r"lib/"
r"bin/"
/ keepers outside lambdas and seeded forms, unchanged
(3#)1 2 3
k2:(2*);k2 1 2 3
k3:(|2*);k3 1 2 3
5+/1 2 3
\\
