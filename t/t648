"=== LDU decomposition ==="

"1. Basic 2x2"
A:(1 2;3 4)
ldu A
mul/ldu A

"2. 3x3"
A:(1 2 3;4 5 6;7 8 10)
mul/ldu A

"3. Rectangular tall"
A:(1 2;3 4;5 6)
mul/ldu A

"4. Rectangular wide"
A:(1 2 3;4 5 6)
mul/ldu A

"5. Singular (rank 2)"
A:(1 2 3;4 5 6;7 8 9)
mul/ldu A

"6. Non-finite -> NaN"
ldu (1 2;0n 4)
ldu (1 2;0i 4)

"=== RREF ==="

"7. Full rank 2x2 -> identity"
rref (1 2;3 4)

"8. Rank deficient 3x3"
rref (1 2 3;4 5 6;7 8 9)

"9. Augmented matrix (solving Ax=b)"
rref (1 2 5;3 4 11)

"10. Wide matrix"
rref (1 2 3;4 5 6)

"11. Tall matrix"
rref (1 2;3 4;5 6)

"12. Non-finite -> NaN"
rref (1 2;0n 4)
rref (1 2;0i 4)
rref (1 2;0N 4)

\\
