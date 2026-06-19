/ RETURN-flag / REPL-mode regressions (file vs stdin must agree).
/ These only misbehaved when read from stdin (REPL mode); file mode was
/ always correct, so they must be checked in BOTH modes (see s.sh).
/ bug1: top-level :return stops the statement list
:9;7
1.0;:2.0;3.0;4.0
/ bug2: a called function's :return must not leak into the caller's expr
/ (cover both prefix `g 5` and bracket `g[5]` apply forms)
g:{:x*2};g 5
g:{:x*2};g[5]
g:{:x*2};1+g 5
g:{:x*2};1+g[5]
f:{:5};2+f[]
g:{:x*2};h:{10+g x};h 3
/ a function returning inside a caller's while loop must not break the loop
g:{:x*10};{s:0;i:0;while[i<3;i+:1;s+:g i];s}[]
\\
