/ caller locals must survive a recursive call whose frames assign different
/ locals: activations share the scope-keys vector, so the callee's first
/ assignments used to rename the caller's local slots (fne_/fne_fast tail
/ snapshot).  value error on the renamed name, or silently the wrong slot.
t6:{[dep] if[dep>2; :99]
 if[0<dep!2; v:1+t6 dep+1; :v]
 k:0
 while[k<2; b:0+k+t6 dep+1; k:k+1]
 k}
t6 0
/ early-return of an if-assigned local after recursion (the day-23 shape)
w:{[m] if[m=0; :5]; if[m=1; q1:11; r1:w 0; :q1]; z1:22; z2:w 1; z3:w 1; z1}
w 2
/ silent wrong-slot alias: callee's c lands on caller's a slot; a duplicate
/ key would make c resolve to a's value (7) instead of its own (9)
v1:{[m] if[m=1; c:100; :c]; a:7; c:9; r:v1 1; c+0*r}
v1 0
/ >32-local tail exercises the xmalloc fallback in the snapshot
big:{[m] if[m=1; z1:1; :z1]
 a0:0; a1:1; a2:2; a3:3; a4:4; a5:5; a6:6; a7:7; a8:8; a9:9
 b0:0; b1:1; b2:2; b3:3; b4:4; b5:5; b6:6; b7:7; b8:8; b9:9
 c0:0; c1:1; c2:2; c3:3; c4:4; c5:5; c6:6; c7:7; c8:8; c9:9
 d0:0; d1:1; d2:2; d3:3; d4:33
 r:big 1; a0+d4}
big 0
/ dict keys are validated at dset: letter, then letters/digits/underscores
.[{[u] d:.(); d[`$"a-b"]:9; 0};,nul;:]
.[{[u] d:.(); d[`$"9x"]:9; 0};,nul;:]
.[{[u] d:.(); d[`$"_ab"]:9; 0};,nul;:]
.[{[u] d:.(); d[`$"ok2"]:9; 0};,nul;:]
.[{[u] d:.(); d[`$"A_B"]:9; 0};,nul;:]
\\
