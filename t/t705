/ comment-only lines inside multi-line lambdas: must lex like empty lines
f:{[a;b] / comment after param list on opening line
a+b}
f[1;2]
g:{[a;b]
/ comment-only body line
a+b}
g[3;4]
h:{ / comment after bare brace
x*y}
h[3;5]
k:{[a;b]
 / leading-space comment-only line
a-b}
k . (9;4)
w:{[a;b]
a+b / trailing comment on code line still fine
}
w[1;1]
\\
