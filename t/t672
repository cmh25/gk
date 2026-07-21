\e 0
/ integer div by zero -> infinity, exactly as % does (it used to raise domain,
/ so two integer ops with a zero divisor disagreed).  Sign follows the
/ numerator; 0 div 0 is null, as 0%0 is 0n.  A long operand anywhere gives 0Ij.
5 div 0
5 div 0j
5j div 0
5j div 0j
/ zero over zero -> null, at the promoted width
0 div 0
0j div 0
0 div 0j
0j div 0j
/ the sign follows the numerator, like %
-5 div 0
-5j div 0
-5%0
5%0
0%0
/ types: any long operand promotes
4:5 div 0
4:5 div 0j
4:5j div 0
/ vectors
5 6 7 div 0
1 2 3 div 1 0 -1
5 div 0 1 0j
/ integer INT_MIN/-1 overflow -> range (a real overflow, not a zero divisor)
.[div;(0N;-1);:]
.[div;(0Nj;-1);:]
/ mod-by-zero and float div-by-zero are NOT errors either
7!0
7%0
/ normal integer division still works
7 div 2
-7 div 2
7 div 2j
/ and/or/xor share div's macro and must be untouched by the zero handling
5 and 0
5 or 0
5 xor 0
12 and 10
12 or 10
12 xor 10
\\
