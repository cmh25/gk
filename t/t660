/ 64-bit int (long/j) -- compare / min-max / mod / match type matrix

/ --- less (<) : returns int bool ---
2j<3j
3j<2j
2j<3
2<3j
2j<3.0
2.0<3j
1 2 3j<2 2 2j
1 2 3j<2
2<1 2 3j

/ --- more (>) ---
3j>2j
3j>2
3>2j
3.0>2j
3 2 1j>2 2 2j

/ --- equal (=) : numeric, cross-type ---
2j=2j
2j=2
2j=2.0
2=2j
1 2 3j=1 9 3j
1 2 3j=2

/ --- min (&) : promoted numeric ---
3j&5j
3j&5
3&5j
3j&5.0
5.0&3j
3 5j&4 4j
3 5j&4
3 5j&4.5

/ --- max (|) ---
3j|5j
3j|5
3|5j
3j|5.0
3 5j|4 4j
3 5j|4

/ --- mod (!) ---
7j!3j
7j!3
7!3j
7 8 9j!3j
7 8 9j!3
-7j!3j

/ --- match (~) : type-strict identity ---
2j~2j
2j~2
2j~2.0
1 2 3j~1 2 3j
1 2 3j~1 2 3
0Nj~0Nj
0Ij~0Ij

/ --- edge: sentinels ordering / min-max ---
0Nj<0j
0Ij>0j
0Nj&5j
0Ij|5j
0Nj=0Nj
-0Ij<0Nj

\\
