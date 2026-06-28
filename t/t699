/ Coverage (bucket A): number theory (nt.c gcd/lcm/modinv) + bit verbs (b.c
/ rot/shift), int+long, atom+vector, plus domain/type error arms. Self-validating
/ via math identities (gcd*lcm=a*b, a*inv mod m=1, shift=*2^n, factor product=n).
/ -- gcd*lcm = a*b (int & long, atom & vector)
&/((12 18 24 gcd 8 6 30)*12 18 24 lcm 8 6 30)=(12 18 24)*8 6 30
&/((12j 18j gcd 8j 6j)*12j 18j lcm 8j 6j)=(12j 18j)*8j 6j
((42 gcd 56)*42 lcm 56)=42*56
/ -- gcd commutative + identities (a gcd 0=a, a gcd a=a)
&/(12 18 24 gcd 8 6 30)=8 6 30 gcd 12 18 24
&/(7 11 13 gcd 0)=7 11 13
&/(7 11 13 gcd 7 11 13)=7 11 13
/ -- modinv: (a * a modinv m) mod m = 1  (coprime; int vec & long)
&/((3 5 7*3 5 7 modinv 11)!11)=1
((3j*3j modinv 11j)!11j)=1
/ -- shift = *2^n; left then right round-trips
&/(5 shift 0 1 2 3)=5*1 2 4 8
&/((5 shift 1 2 3) shift -1 -2 -3)=5 5 5
/ -- rot round-trips (rotate by n then -n)
&/((123 rot 1 2 3) rot -1 -2 -3)=123 123 123
/ -- prime / factor: product of factors = n; all factors prime
(*/factor 360)=360
&/prime factor 360
&/prime 2 3 5 7 11 13
&/~prime 4 6 8 9 10
/ -- error arms (trap -> first elt of (1;err) is 1)
*@[{4 modinv 8};0;:]
*@[{12.0 gcd 18};0;:]
*@[{2.5 shift 3};0;:]
\\
