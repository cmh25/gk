/ Closed projections and dicts of callables capture lexically too.  A projection
/ (0xd9) or adverb-wrapper (0xda) carries its inner lambda's captured scope when
/ returned/copied, and a dict whose every entry is a lambda/projection shares one
/ captured scope.  Primitives/builtins project but capture nothing (pass through).
/ Regression for the closure_any / proj_captures return-path extension.
mk:{[cc] c:cc; {[a;b]a+b+c}[10]}
p:mk 5
p 2                                   / 17 -- returned closed projection
{c:999; p 2}[]                        / 17 -- applied through a shadowing scope
mk2:{c:x; {[a;b;d]a+b+d+c}[100][5]}   / projection of a projection
q:mk2 1
q 0                                   / 106
inner:{[cc] c:cc; {[a;b]a+b+c}}       / projection of an already-closed lambda
mk3:{f:inner x; f[10]}
{c:999; (mk3 5) 2}[]                  / 17
mkc:{n:0; {[k;j]n+:k+j;n}[10]}        / a projection that MUTATES captured state
cnt:mkc[]
cnt 5                                 / 15
cnt 5                                 / 30 -- state persists across calls
/ dict whose entries are a lambda AND a projection, sharing one captured scope
obj:{i:0; d:.(); d.get:{i}; d.add:{[k;j]i+:k+j;i}[1]; d}[]
obj.get`                              / 0
obj.add 4                             / 5   (i += 1+4)
obj.get`                              / 5   shared state
/ primitives/builtins project but capture nothing -- unchanged, no crash
pp:{+[x;]}[3]
pp 4                                  / 7   returned primitive projection, no capture needed
/ regressions: bare lambda closures still lexical
inr:{[f] c:777; f[]}
{c:x; inr[{c}]}[5]                    / 5
cf:{i:0;{i+:1;i}}
g:cf`
g`                                    / 1
g`                                    / 2
\\
