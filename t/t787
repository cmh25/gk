/ Final i64/control audit.

dm:{[f;x](1;"domain")~@[f;x;:]}
ln:{[f;x](1;"length")~@[f;x;:]}
ty:{[f;x](1;"type")~@[f;x;:]}

/ A long-zero predicate is false; nonzero long predicates enter the body.
0~{0j}{x+`a}/0
(,0)~{0j}{x+`a}\0
ty[{{1j}{x+`a}/x};0]
ty[{{1j}{x+`a}\x};0]

/ Slide accepts finite i64 strides and rejects i64 sentinels like i32 ones.
_[1;,;"abcd"]~_[1j;,;"abcd"]
_[-2;-;4 3 2 1]~_[-2j;-;4 3 2 1]
(,3)~_[3000000000j;+;1 2]
ln[{_[3000000000j;,;x]};1 2 3]
dm[{_[x;,;1 2]};0Nj]
dm[{_[x;,;1 2]};0Ij]
dm[{_[x;,;1 2]};-0Ij]

/ All file-slice readers accept finite i64 offsets/counts.
("c"1:("b3";4;4))~"c"1:("b3";4j;4j)
(("cc";1 1)1:("b3";4;4))~("cc";1 1)1:("b3";4j;4j)
(("IFCS";7 9 10 3)0:("a.txt";30;30))~("IFCS";7 9 10 3)0:("a.txt";30j;30j)
/ Real offsets/counts follow the same finite numeric rule as float.
("c"1:("b3";4;4))~"c"1:("b3";4.0e;4.0e)
(("cc";1 1)1:("b3";4;4))~("cc";1 1)1:("b3";4.0e;4.0e)
(("IFCS";7 9 10 3)0:("a.txt";30;30))~("IFCS";7 9 10 3)0:("a.txt";30.0e;30.0e)
ln[{"c"1:("b3";x;0j)};3000000000j]
dm[{"c"1:("b3";x;0j)};0Nj]
dm[{"c"1:("b3";x;0j)};0Ij]
dm[{"c"1:("b3";x;0j)};-0Ij]

/ Numeric atom subtypes are interchangeable as zero-duration sleeps.
nul~sleep 0j
nul~sleep 0.0e

\\
