/ Coverage (bucket A): gcd/lcm/rot/shift dispatch arms (nt.c gcd_/lcm_, b.c rot_/shift_).
/ Integer-only. Each cell compares a combo to its type-flipped (int<->long) twin on the same
/ values: covers every int/long x atom/vector arm (incl general-list via kv) and asserts the
/ typings agree. One assertion per line so a 0 pinpoints the arm. (A few general-list cells
/ print a 2D block of 1s -- that verb distributes a list operand as an each.) rot/shift are
/ bit ops (a shift x=a<<x, a rot x=bit-rotate; left=value, right=count).
/ -- gcd: int/long x atom/vector (cell vs type-flipped twin)
(12 gcd 8)=12j gcd 8j
(12 gcd 8j)=12j gcd 8
(12j gcd 8)=12 gcd 8j
(12j gcd 8j)=12 gcd 8
(12 gcd 8 6 9)=12j gcd 8 6 9j
(12 gcd 8 6 9j)=12j gcd 8 6 9
(12j gcd 8 6 9)=12 gcd 8 6 9j
(12j gcd 8 6 9j)=12 gcd 8 6 9
(12 18 24 gcd 8)=12 18 24j gcd 8j
(12 18 24 gcd 8j)=12 18 24j gcd 8
(12 18 24j gcd 8)=12 18 24 gcd 8j
(12 18 24j gcd 8j)=12 18 24 gcd 8
(12 18 24 gcd 8 6 9)=12 18 24j gcd 8 6 9j
(12 18 24 gcd 8 6 9j)=12 18 24j gcd 8 6 9
(12 18 24j gcd 8 6 9)=12 18 24 gcd 8 6 9j
(12 18 24j gcd 8 6 9j)=12 18 24 gcd 8 6 9
/ -- gcd: general-list (case 0) arms via kv
&/,/(5 6 7 gcd kv 0 1 2)=5 6 7j gcd kv 0 1 2
(5 gcd kv 0 1 2)=5j gcd kv 0 1 2
((kv 5 6 7) gcd 0 1 2)=(kv 5 6 7) gcd 0 1 2j
((kv 5 6 7) gcd 2)=(kv 5 6 7) gcd 2j
&/,/((kv 5 6 7) gcd kv 0 1 2)=(kv 5 6 7j) gcd kv 0 1 2
/ -- lcm: int/long x atom/vector (cell vs type-flipped twin)
(12 lcm 8)=12j lcm 8j
(12 lcm 8j)=12j lcm 8
(12j lcm 8)=12 lcm 8j
(12j lcm 8j)=12 lcm 8
(12 lcm 8 6 9)=12j lcm 8 6 9j
(12 lcm 8 6 9j)=12j lcm 8 6 9
(12j lcm 8 6 9)=12 lcm 8 6 9j
(12j lcm 8 6 9j)=12 lcm 8 6 9
(12 18 24 lcm 8)=12 18 24j lcm 8j
(12 18 24 lcm 8j)=12 18 24j lcm 8
(12 18 24j lcm 8)=12 18 24 lcm 8j
(12 18 24j lcm 8j)=12 18 24 lcm 8
(12 18 24 lcm 8 6 9)=12 18 24j lcm 8 6 9j
(12 18 24 lcm 8 6 9j)=12 18 24j lcm 8 6 9
(12 18 24j lcm 8 6 9)=12 18 24 lcm 8 6 9j
(12 18 24j lcm 8 6 9j)=12 18 24 lcm 8 6 9
/ -- lcm: general-list (case 0) arms via kv
&/,/(5 6 7 lcm kv 0 1 2)=5 6 7j lcm kv 0 1 2
(5 lcm kv 0 1 2)=5j lcm kv 0 1 2
((kv 5 6 7) lcm 0 1 2)=(kv 5 6 7) lcm 0 1 2j
((kv 5 6 7) lcm 2)=(kv 5 6 7) lcm 2j
&/,/((kv 5 6 7) lcm kv 0 1 2)=(kv 5 6 7j) lcm kv 0 1 2
/ -- rot: int/long x atom/vector (cell vs type-flipped twin)
(5 rot 2)=5j rot 2j
(5 rot 2j)=5j rot 2
(5j rot 2)=5 rot 2j
(5j rot 2j)=5 rot 2
(5 rot 0 1 2)=5j rot 0 1 2j
(5 rot 0 1 2j)=5j rot 0 1 2
(5j rot 0 1 2)=5 rot 0 1 2j
(5j rot 0 1 2j)=5 rot 0 1 2
(5 6 7 rot 2)=5 6 7j rot 2j
(5 6 7 rot 2j)=5 6 7j rot 2
(5 6 7j rot 2)=5 6 7 rot 2j
(5 6 7j rot 2j)=5 6 7 rot 2
(5 6 7 rot 0 1 2)=5 6 7j rot 0 1 2j
(5 6 7 rot 0 1 2j)=5 6 7j rot 0 1 2
(5 6 7j rot 0 1 2)=5 6 7 rot 0 1 2j
(5 6 7j rot 0 1 2j)=5 6 7 rot 0 1 2
/ -- rot: general-list (case 0) arms via kv
(5 6 7 rot kv 0 1 2)=5 6 7j rot kv 0 1 2
(5 rot kv 0 1 2)=5j rot kv 0 1 2
((kv 5 6 7) rot 0 1 2)=(kv 5 6 7) rot 0 1 2j
((kv 5 6 7) rot 2)=(kv 5 6 7) rot 2j
((kv 5 6 7) rot kv 0 1 2)=(kv 5 6 7j) rot kv 0 1 2
/ -- shift: int/long x atom/vector (cell vs type-flipped twin)
(5 shift 2)=5j shift 2j
(5 shift 2j)=5j shift 2
(5j shift 2)=5 shift 2j
(5j shift 2j)=5 shift 2
(5 shift 0 1 2)=5j shift 0 1 2j
(5 shift 0 1 2j)=5j shift 0 1 2
(5j shift 0 1 2)=5 shift 0 1 2j
(5j shift 0 1 2j)=5 shift 0 1 2
(5 6 7 shift 2)=5 6 7j shift 2j
(5 6 7 shift 2j)=5 6 7j shift 2
(5 6 7j shift 2)=5 6 7 shift 2j
(5 6 7j shift 2j)=5 6 7 shift 2
(5 6 7 shift 0 1 2)=5 6 7j shift 0 1 2j
(5 6 7 shift 0 1 2j)=5 6 7j shift 0 1 2
(5 6 7j shift 0 1 2)=5 6 7 shift 0 1 2j
(5 6 7j shift 0 1 2j)=5 6 7 shift 0 1 2
/ -- shift: general-list (case 0) arms via kv
&/,/(5 6 7 shift kv 0 1 2)=5 6 7j shift kv 0 1 2
(5 shift kv 0 1 2)=5j shift kv 0 1 2
((kv 5 6 7) shift 0 1 2)=(kv 5 6 7) shift 0 1 2j
((kv 5 6 7) shift 2)=(kv 5 6 7) shift 2j
&/,/((kv 5 6 7) shift kv 0 1 2)=(kv 5 6 7j) shift kv 0 1 2
/ -- inner default arms: integer value, float count -> type error
*@[{5 rot 2.5};0;:]
*@[{5j rot 2.5};0;:]
*@[{5 6 7 rot 2.5};0;:]
*@[{5 6 7j rot 2.5};0;:]
*@[{5 shift 2.5};0;:]
*@[{5j shift 2.5};0;:]
*@[{5 6 7 shift 2.5};0;:]
*@[{5 6 7j shift 2.5};0;:]
/ -- call-form equivalence: infix / bracket / projection agree (incl long)
(12 gcd 8j)=gcd[12;8j]
(gcd[12;8j])=gcd[12]8j
(12 shift 3j)=shift[12;3j]
(shift[12;3j])=shift[12]3j
/ -- error arms: float / symbol rejected (default: KERR_TYPE)
*@[{12.0 gcd 6};0;:]
*@[{2.5 shift 3};0;:]
*@[{1.5 lcm 6};0;:]
*@[{2.5 rot 3};0;:]
*@[{`a shift 2};0;:]
\\
