/ Integer sentinels used as structural controls must retain their meaning
/ when widened. An explicit finite long at the i32 boundary remains distinct.

ws:{[f;x](1;"wsfull")~@[f;x;:]}
dm:{[f;x](1;"domain")~@[f;x;:]}
ty:{[f;x](1;"type")~@[f;x;:]}
ln:{[f;x](1;"length")~@[f;x;:]}

/ Take: null is the documented identity; infinities cannot become enormous
/ finite int counts. Multi-dimensional shapes use the same widening rule.
(0N#"abc")~0Nj#"abc"
ws[{x#,"a"};0I]
ws[{x#,"a"};-0I]
ws[{x#,"a"};0Ij]
ws[{x#,"a"};-0Ij]
ws[{(x 0)#x 1};(0 0I;1)]
ws[{(x 0)#x 1};(0 0Ij;1)]
dm[{(x 0)#x 1};(0 -0I;1)]
dm[{(x 0)#x 1};(0 -0Ij;1)]

/ Enumerate rejects infinite counts. Draw retains positive infinity as a
/ valid range bound; null and negative infinity remain invalid bounds.
dm[{!x};0I]
dm[{!x};0Ij]
ws[{(x 0)?x 1};(0I;-1)]
ws[{(x 0)?x 1};(0Ij;-1j)]
ws[{0?x};0N]
3=#3?0I
ws[{0?x};-0I]
ws[{0j?x};0Nj]
3=#3?0Ij
ws[{0j?x};-0Ij]
3=#draw[3;0I]
3=#draw[3;0Ij]
dm[{draw[x;10]};0 0I]

/ The maximum encoded int/long counts are valid controllers for do, over, and
/ scan. A body error proves execution starts instead of an up-front `limit`.
ty[{do[x;1+`a]};0I]
ty[{do[x;1+`a]};0Ij]
ty[{while[x;1+`a]};0I]
ty[{while[x;1+`a]};0Ij]
ty[{x{x+`a}/0};0I]
ty[{x{x+`a}/0};0Ij]
ty[{x{x+`a}\0};0I]
ty[{x{x+`a}\0};0Ij]

/ Predicate-controlled primitive over and scan likewise enter their bodies.
ty[{{1}{x+`a}/x};0]
ty[{{1}{x+`a}\x};0]

/ A long-vector shape is a shape, just as an int vector is.
d:2 2j draw 10j
(2 2)~^d
-8=4:*d

/ A slide stride is finite and non-null; otherwise a two-item input used to
/ make a sentinel look accidentally valid because only one pair was needed.
dm[{_[x;,;1 2]};0N]
dm[{_[x;,;1 2]};0I]
dm[{_[x;,;1 2]};-0I]

/ File slice offsets and lengths obey the same finite-control rule. The
/ schema path also used to read a boxed float through the int converter.
("46";"57")~("cc";1 1)1:("b3";4.0;4.0)
dm[{"c"1:("b3";x;0)};0I]
dm[{"c"1:("b3";x;0)};0i]
dm[{"c"1:("b3";0;x)};0I]
dm[{"c"1:("b3";0;x)};0i]
dm[{("cc";1 1)1:("b3";x;0)};0I]
dm[{("IFCS";7 9 10 3)0:("a.txt";x;0)};0I]
ln[{("CC";x,1)1:("b3";0;0)};0I]
ln[{("CCC";1431655770 1431655768 1431655768)1:("b3";0;10)};0]

\\
