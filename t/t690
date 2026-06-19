/ draw (named builtin) must accept i64 (long) counts -- was a type error.
/ Regression: `3j draw 10` (count is a long atom). The named draw now
/ delegates long counts to ? (v.c draw), which handles i64-range and
/ big-vector validation, so draw and ? agree on shape/type.
#3j draw 10
#0j draw 10
#5j draw -5
d:5j draw -5;(!5)~d@<d
#100j draw 1.5
(@3j draw 10)~@3j?10
(@3j draw -5)~@3j?-5
\\
