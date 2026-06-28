/ Coverage: drop/cut (k.core/v.c `drop`, was 53%). Cut forms (a _ x with a an
/ index VECTOR -> partition) across every x element-type with int and long cut-
/ points, plus drop-by-long-count edges. Self-validating: long result must match
/ (~) the int result (drop/cut preserve element type, so ~ is exact).
/ -- cut: long index-vector matches int index-vector, x all element-types
(0 2 4j _ "abcdefg")~0 2 4 _ "abcdefg"
(0 2 4j _ 1 2 3 4 5 6 7)~0 2 4 _ 1 2 3 4 5 6 7
(0 2 4j _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5)~0 2 4 _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5
(0 2 4j _ 1 2 3 4 5 6 7j)~0 2 4 _ 1 2 3 4 5 6 7j
(0 2 4j _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5e)~0 2 4 _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5e
(0 2 4j _ `a`b`c`d`e`f`g)~0 2 4 _ `a`b`c`d`e`f`g
(0 2 4j _ (1 2;3;4 5;6;7;8 9;0))~0 2 4 _ (1 2;3;4 5;6;7;8 9;0)
/ -- drop-by-long matches drop-by-int (pos, neg) x all element-types
(2j _ "abcdefg")~2 _ "abcdefg"
(-2j _ "abcdefg")~-2 _ "abcdefg"
(2j _ 1 2 3 4 5 6 7)~2 _ 1 2 3 4 5 6 7
(-2j _ 1 2 3 4 5 6 7)~-2 _ 1 2 3 4 5 6 7
(2j _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5)~2 _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5
(-2j _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5)~-2 _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5
(2j _ 1 2 3 4 5 6 7j)~2 _ 1 2 3 4 5 6 7j
(-2j _ 1 2 3 4 5 6 7j)~-2 _ 1 2 3 4 5 6 7j
(2j _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5e)~2 _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5e
(-2j _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5e)~-2 _ 1.5 2.5 3.5 4.5 5.5 6.5 7.5e
(2j _ `a`b`c`d`e`f`g)~2 _ `a`b`c`d`e`f`g
(-2j _ `a`b`c`d`e`f`g)~-2 _ `a`b`c`d`e`f`g
(2j _ (1 2;3;4 5;6;7;8 9;0))~2 _ (1 2;3;4 5;6;7;8 9;0)
(-2j _ (1 2;3;4 5;6;7;8 9;0))~-2 _ (1 2;3;4 5;6;7;8 9;0)
/ -- drop edges: over-length, zero, drop-from-atom, over-length negative
(9j _ 1 2 3)~9 _ 1 2 3
(0j _ 1 2 3)~0 _ 1 2 3
(2j _ 5)~2 _ 5
(-9j _ 1 2 3)~-9 _ 1 2 3
\\
