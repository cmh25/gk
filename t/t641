/ scope rules regression tests

/ a:1 always local
f:{a:5;r:{a:9;a}`;(a;r)}
f`

/ a::1 resolve then modify (local param shadows)
g:{a:5;r:{a::9;a}`;(a;r)}
g`

/ a+:1 resolve then modify
h:{a:5;r:{a+:1;a}`;(a;r)}
h`

/ closure can modify parent with +: 
makecounter:{n:0;{n+:1;n}}
c:makecounter`
(c`;c`;c`)

/ d.a:1 always local (creates fresh dict)
k:{d:.+(,`x;,1);r:{d.a:9;d}`;(d;r)}
k`

/ d[`a]:1 resolves (modifies global)
d1:.,`a,1
{d1[`a]:99}`
d1

/ d[`a]::1 resolves (modifies global)
d2:.,`b,2
{d2[`b]::88}`
d2

/ d[`a]::1 with local d (modifies local, not global)
d3:.,`c,3
i:{d3:.,`x,0;d3[`x]::77;d3}
(i`;d3)

/ d[`a]+:1 resolves (modifies global)
d4:.,`n,10
{d4[`n]+:5}`
d4

/ d[`a]+:1 with lexical parent (non-closure creates local copy)
j:{d5:.,`m,100;r:{d5[`m]+:1; d5}`;(d5;r)}
j`

\\
