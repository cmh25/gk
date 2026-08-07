/ A predefined k.core function (0xc9/0xca/0xcb) followed by a postfix
/ adverb must stay a function until the adverb is attached.  With an
/ assignment target already on the value stack, the predefined-token case
/ used to consume that target as an argument (`value); after deferring it,
/ the generic 0xda wrapper also had to resolve the raw predefined value.

x:("abc";"def")

/ original report: assigned predefined dyad + each-left + bracket args
r:dv\[x;"b"]
r

/ all three predefined valences, including the 0x45(args+adverb) path
r:gtime'0 1
r~(gtime)'0 1
r:dv'[x;"be"]
r~(dv)'[x;"be"]
r:ssr'[x;"a";"x"]
r~(ssr)'[x;"a";"x"]

/ the other dyadic adverbs use the same modified-function wrapper
(r:dv/["abc";"be"])~(dv)/["abc";"be"]
(r:dv/("abc";"b"))~(dv)/("abc";"b")
(r:dv\("abc";"b"))~(dv)\("abc";"b")

/ chained postfix and assignment contexts
(r:gtime''(0 1;2 3))~(gtime)''(0 1;2 3)
s:r:dv'[x;"be"]
(s~r)&r~(dv)'[x;"be"]
{r:dv\[x;"b"];r~(dv)\[x;"b"]}[]
z:1#,0
z[0]:dv'[x;"be"]
z[0]~(dv)'[x;"be"]

/ assigning the modified function itself must work too
f:dv'
f[x;"be"]~(dv)'[x;"be"]
g:gtime'
(g 0 1)~(gtime)'0 1

/ .r exposes the raw predefined values: these used to build an unsupported
/ 0xda(raw-predefined;adverb) and type-error even without an assignment
.r.dv'[x;"be"]~(dv)'[x;"be"]
(.r.gtime'0 1)~(gtime)'0 1

/ The adjacent audit found the same missing defer/wrapper support for file
/ verbs.  Unbracketed 5:'x took a special direct path and already worked;
/ bracket application and storing the modified verb did not.
(r:5:'[!3])~{5:x}'[!3]
(5:'[!3])~{5:x}'[!3]
h:5:'
h[!3]~{5:x}'[!3]
(h(!3))~{5:x}'[!3]
\\
