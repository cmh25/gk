/ Closure capture is LEXICAL: a lambda's free variables resolve against its
/ DEFINING scope, and that binding is preserved when the lambda is copied --
/ passed to another function, returned, or stored -- even when the copying
/ scope has a same-named local.  Regression for the fncp_ scope-preservation
/ fix (the closure flag ps[3] / captured parent was being discarded on copy,
/ giving dynamic scope: closures reachable only when returned+called at top).
inner:{[f] c:777; f[]}
{c:x; inner[{c}]}[5]                 / 5  -- not inner's decoy c=777
mk:{c:x;{c}}
use:{[f] c:999; f[]}
use[mk 5]                            / 5  -- returned closure survives copy thru decoy
{[f] q:1; f[]}[mk 7]                 / 7  -- caller lacks c: still finds the capture
find:{[u;pr] r:0;t:0;while[t<3;a:0;if[pr t;r:t];t:t+1];r}
{[a] L9:a; find[0;{x=L9}]}[2]        / 2  -- closure to a finder with locals + a loop
ap:{[f] z:1;f[]}
mid:{[f] c:99;d:88;ap[f]}
{c:x;d:y;mid[{c+d}]}[10;20]          / 30 -- two captures, passed two deep, decoys shadow
/ documented closures must still hold (doc/v1.md):
f:{i:0;{i+:1;i}}
g:f`
g`                                   / 1
g`                                   / 2
(f`)`                                / 1  -- a fresh, independent closure
h:{i:j:0;r.inci:{(i+:1;j)};r.incj:{(i;j+:1)};r}
o:h`
o.inci`                              / 1 0
o.inci`                              / 2 0
o.incj`                              / 2 1
/ lexical, NOT dynamic: a top-level fn cannot see a caller's local (doc example).
/ error-trapped so the script does not abort.
G0:{a}
.[{[u] a:1; G0`};,nul;:]             / (1;"value") -- G0 can't resolve caller's a
\\
