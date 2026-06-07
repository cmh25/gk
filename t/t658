/ 64-bit int (long / j) type

/ type-of: 8 atom, -8 vector
4:1j
4:0 1 2j
4:0j

/ literals and sentinels
1j
0 1 2 3j
0Nj
0Ij
-0Ij
0Nj 0Ij -0Ij
1000000000000j
1000000000j*1000000000

/ promotion matrix (int/long/float)
4:1j+1
4:1+1j
4:1j+1j
4:1j+1.0
4:1.0+1j
1j+1
1j+1j
1j+1.5
0j+1 2 3
1 2 3+0j

/ arithmetic
1 2 3j+10 20 30j
5j-2j
3j*4j
7j%2j
7j!3j
2j^10j
-5j
abs -5j
abs 0Nj

/ compare / min / max
3j<5j
3 5 7j>4 4 4j
1 2 3j=1 9 3j
3 5j&4 4j
3 5j|4 4j

/ structural
1 2 3j,4 5j
|1 2 3j
1 2 3 4 5j[1 3]
2j#9j
2_0 1 2 3 4j
2j#1 2 3 4 5 6j
*1 2 3j
&0 1 0 2j
#1 2 3 4j

/ search / group / grade
?1 1 2 3 3j
1 2 3j?2j
=1 2 1 3 2j
<3 1 2j
>3 1 2j
/ grade with ties (stable) -- counting-sort tier
<3 1 2 1 3j
>3 1 2 1 3j
/ grade wide range > 256M -- merge-sort fallback tier
<1000000000000 1 5 1000000000000 0Nj
1000000000000 1 5 1000000000000 0Nj@<1000000000000 1 5 1000000000000 0Nj

/ reduce / scan / each
+/1 2 3 4j
*/1 2 3 4j
|/3 1 4 1 5j
&/3 1 4 1 5j
+\1 2 3 4j
{x*2}'1 2 3j

/ monadic primitives: negate(-) recip(%) atom(@) flip(+)
- 5j
- 1 2 3j
- 0Ij
%5j
%1 2 4j
4:%5j
@5j
@1 2 3j
+(1 2j;3 4j)

/ not / floor
~1 0 2j
_5j
_1 2 3j

/ match
1 2 3j~1 2 3j
1 2 3j~1 2 3
1j~1

/ enumerate (!) of a long count yields a long vector
4:!10j
!5j
4:!10

/ roll / deal (?) with a long modulus yields longs
4:3?100j
4:3?10000000000j
#?100?-100j

\\
