/ top-k grade fusion (k#<x, k#>x, y@k#>x) on longs and floats: the long
/ quickselect and float key-select fallbacks must match the unfused forms,
/ including NaN, signed zero, int64 extremes and wide value spreads.  The
/ float min/max fold keeps NaN as the minimum and gives ties to the last
/ tying element (so the sign of a zero result is the last zero's).
w:1000003!7919*0j+!_20000
(100#>w)~100#(>w)
(100#<w)~100#(<w)
(w@100#>w)~w@100#(>w)
w2:(0j+!_2000)*4000000000j
(100#>w2)~100#(>w2)
(100#<w2)~100#(<w2)
w3:(2000?10j),9223372036854775807j,-9223372036854775807j
(100#>w3)~100#(>w3)
(100#<w3)~100#(<w3)
f:0.0+w
(100#>f)~100#(>f)
(100#<f)~100#(<f)
v:200#(0n;0.0*-1;0.0;1%0.0;-1%0.0;3.5;-2.5;0n;7.25)
(20#>v)~20#(>v)
(20#<v)~20#(<v)
&/v
|/v
1%&/(0.0;0.0*-1)
1%&/(0.0*-1;0.0)
1%|/(0.0;0.0*-1)
&/1.5 -2.5 0n
|/1.5 -2.5 0n
\\
