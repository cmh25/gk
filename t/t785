/ Follow-up correctness regressions: integer widening in bin/vs/sv, plus
/ POSIX shell-child signal and lifecycle hygiene.

/ Integer bin/binl must compare i32 sentinels as their i64 counterparts in
/ both the haystack and the needle.
0N -100 bin -3000000000j
0N -100 binl -3000000000j
1 0I bin 3000000000j
1 0I binl 3000000000j

/ A vs/sv operation promoted to long must widen i32 atoms and vector elements
/ sentinel-aware. Pair each narrow value with its explicit long spelling.
(2j vs 0N)~2j vs 0Nj
(2j vs 0N 0I -0I)~2j vs 0Nj 0Ij -0Ij
(2j sv 0N 0I -0I)~2j sv 0Nj 0Ij -0Ij

/ Radices are finite ordinary integers. Promotion must preserve rejection of
/ all three integer sentinels, for scalar and mixed radices in both directions.
domvs:{[b;x](1;"domain")~@[{vs[x 0;x 1]};(b;x);:]}
domsv:{[b;x](1;"domain")~@[{sv[x 0;x 1]};(b;x);:]}
domvs[0N;3000000000j]
domvs[0I;300000000j]
domvs[0I;3000000000j]
domvs[-0I;3000000000j]
domvs[0Nj;3000000000j]
domvs[0Ij;3000000000j]
domvs[-0Ij;3000000000j]
domvs[0 0N 2;3000000000j]
domvs[0 0I 2;300000000j]
domvs[0 0I 2;3000000000j]
domvs[0 -0I 2;3000000000j]
domvs[0j 0Ij 2j;3000000000j]
domsv[0N;1j 2j]
domsv[0I;1 2]
domsv[0I;1j 2j]
domsv[-0I;1j 2j]
domsv[0Nj;1j 2j]
domsv[0Ij;1j 2j]
domsv[-0Ij;1j 2j]
domsv[0 0N 2;1j 1j 1j]
domsv[0 0I 2;1 1 1]
domsv[0 0I 2;1j 1j 1j]
domsv[0 -0I 2;1j 1j 1j]
domsv[0j 0Ij 2j;1j 1j 1j]

/ Round-trip both sides of every pair. For vs inputs, widen the expected value
/ to long because every case selects the long implementation. For sv inputs,
/ round-trip the decoded value: arbitrary input digits need not be canonical
/ (for example, sentinels are not base-2 digits).
vrt:{[b;x](0j+x)~b sv b vs x}
srt:{[b;x]v:b sv x;v~b sv b vs v}
vrt[2j;0N]
vrt[2j;0Nj]
vrt[2j;0N 0I -0I]
vrt[2j;0Nj 0Ij -0Ij]
srt[2j;0N 0I -0I]
srt[2j;0Nj 0Ij -0Ij]

/ main ignores SIGPIPE for IPC, but an exec'd shell must see the default.
/ Async shell children must also be reaped. The ps/awk probe is POSIX-only.
win:"Windows_NT"~getenv "OS"
$[win;1;{r:`8:"kill -PIPE $$; echo survived";141=r 0}[]]
$[win;1;{`3:"true";`4:"sleep .2";c:"ps -axo ppid=,stat= | awk '$1==",($.z.P)," && $2~/^Z/{n++} END{print n+0}'";r:`8:c;(,"0")~*(r 1)}[]]
\\
