\l load.k
\d msg
r:box["hello from gk on Windows!"]
"box returned: ",$r
if[~r~1; `0:"FAIL: expected IDOK (1)"; exit 1]
"ok"
\\
