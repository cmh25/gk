/ quiet values: the value of an assignment is not echoed at the prompt, and
/ that quietness travels with the value through anything that passes it along
/ unchanged -- a lambda's return (every call path), a $[] branch, an eval, a
/ parenthesised expression -- while anything that computes a new value from
/ it is loud.  The flag used to be reset per token only, so a call or a $[]
/ as the left operand of a verb kept the statement silent ((f 1)+1 printed
/ nothing), the bracket fast path and $[] never propagated (f[1] and
/ $[1;a:3;0] echoed while f 1 did not), an adverb over such a lambda was
/ silent, and . threw the value away (r:."a:6" left r as nul).
{a:1}`
r:{a:2}`
r
$[1;a:3;0]
r:$[1;a:4;0]
r
."a:5"
r:."a:6"
r
r~6
4:."a:1"
/ every call path of f:{a:x} is quiet; a verb applied to the result is loud
f:{a:x}
f 1
f[1]
(f 1)
f@1
f .,1
.[f;,1]
@[f;1]
(f 1)+1
1+f 1
f[1]+1
$[1;a:3;0]+1
/ adverbs compute a new value
f'1 2
{a:x;x}'1 2
+/f'1 2
/ parens pass a value along; a list is a new value
(a:3)
(a:3;4)
(a:3)+1
-a:5
#a:1 2 3
*(a:1 2)
x:a:7
x
/ the selected-region return of an indexed assign in a value context is loud
g:{x[1 2]:0}
(g 4 5 6)~0 0
{x[0]:8;x} 1 2 3
/ amend is a new value even when it fires a watch whose trigger assigns
v:1 2 3
n:0
watch(`v;"n+:1")
.[`v;0;:;9]
@[`v;1;:;8]
v
n
watch(`v;"")
\\
