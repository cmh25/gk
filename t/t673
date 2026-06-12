\e 0
/ --- variable watches (triggers) ---
/ the empty symbol is not a variable name: domain error, nothing armed
watch(`;"c+:1")
/ a non-symbol first element is a type error, nothing armed
watch("a";"c+:1")
watch(,"a";"c+:1")
watch(2;"c+:1")
watch(2.0;"c+:1")
watch(2.0e;"c+:1")
watch`
/ basic: trigger runs after each assignment to the watched var
a:b:0
watch(`a;"b+:1")
b
a:1
b
a:1
b
/ amend (+:) fires too
a+:1
b
/ query: list armed watches
c:0
watch(`c;"b+:10")
watch`
/ clear one watch; the other stays
watch(`a;"")
watch`
a:99
b
c:1
b
/ self-reference fires once, not forever
s:0
watch(`s;"s+:1")
s:5
s
/ amend-by-reference (.[`v;...] / @[`v;...]) fires, dotted or not
r:0
v:1 2 3
watch(`v;"r+:1")
@[`v;0;:;7]
v
r
@[`.k.v;1;:;8]
v
r
/ assigning a member of a dict var fires that var's watch, relative or absolute
da:.,`x,1
dc:0
watch(`da;"dc+:1")
da.x:2
dc
.k.da.x:3
dc
watch(`da;"")
/ scope isolation: `a` in .foo is independent of `a` in .k
\d .foo
ay:0
watch(`a;"ay+:100")
\d .k
a:1
.foo.ay
/ fully-qualified + cross-namespace assigns fire
.foo.a:7
.foo.ay
.foo.a
\d .k
\\
