/ reshape 0N dimension inference; floor _ and float comparison are exact (no fp tolerance)

/ --- 0N k # x : rows of width k, count inferred, last row ragged ---
0N 2#"asdf1234"
0N 3#"asdf1234"
0N 3#!7
0N 2#!8
/ --- r 0N # x : exactly r rows, split evenly (remainder by floor boundaries) ---
2 0N#"asdf1234"
2 0N#"asdf123"
3 0N#"asdf1234"
4 0N#"ab"
1 0N#"asdf"
/ --- edges: empty, r=0, types preserved ---
0N 2#""
2 0N#""
0 0N#"asdf"
2 0N#1 2 3 4 5 6j
/ --- bad shapes -> errors (0N is the only infer marker; -1 is just a negative dim) ---
.[#;(0N 0N;"asdf");:]
.[#;(2 -1;"asdf1234");:]
.[#;(0N 0;"asdf");:]

/ --- the floor verb _ is EXACT
_1.9999999999999
_1.999999999999
_2.0000000000001
_-2.0000000000001
_1.9999999999999 2.9999999999999 3.5
_3.7
_-4.6
/ = is exact too (no fp comparison tolerance)
2=1.9999999999999
2=1.999999999999

/ --- f32 (real): comparison and floor both exact ---
/ (sqrt 2.0e)^2 is 1 ULP off 2.0e: = says different, _ floors to 1
((sqrt 2.0e)*sqrt 2.0e)=2.0e
_(sqrt 2.0e)*sqrt 2.0e
2.0e~(sqrt 2.0e)*sqrt 2.0e
/ distinct f32 values stay distinct
1.0e=1.0001e
1.0e=1.00001e
1.0e=1.000001e
\\
