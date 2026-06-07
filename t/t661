/ 64-bit int (long/j) -- builtins type matrix

/ --- float math monads on long (8 / -8) -> float ---
sqrt 16j
sqrt 0 16 64j
sqr 5j
exp 0j
log 1j
sin 0j
ceil 5j
floor 7j
trunc 7j
rint 3j
sqrt 0Nj
sqrt 0Ij

/ --- atan2 / hypot accept long ---
atan2[0j;1j]
hypot[3j;4j]
hypot[3j;4]
hypot[3 6j;4 8j]

/ --- div : integer division, int/long matrix ---
div[17j;5j]
div[17j;5]
div[17;5j]
div[100 50j;7]
div[-17j;5j]

/ --- bitwise and/or/xor (8 8, 8 1, 1 8, -8 -8, mix) ---
and[12j;10j]
and[12j;10]
and[12;10j]
or[12j;10j]
xor[12j;10j]
and[12 6j;10 3j]
and[12 6j;10 3]
or[1 2 4j;8]
and[12000000000j;10000000000j]

/ --- bitwise not (monad) ---
not 5j
not 0j
not 1 2 4j

/ --- rotate / shift on long values ---
rot[1j;1j]
rot[1j;8]
shift[1j;4j]
shift[256j;-4j]
shift[1j;40j]
shift[1 2 4j;4]
rot[5j;0Nj]

/ --- gcd / lcm / modinv (matrix + no-clamp) ---
gcd[12j;8j]
gcd[12j;8]
gcd[12;8j]
lcm[4j;6j]
modinv[3j;7j]
gcd[12000000000j;8000000000j]
gcd[12 48j;8 18j]

/ --- prime / factor ---
prime 1000000007j
prime 1000000006j
prime 2 3 4 5j
factor 360j
factor 1000000007j
factor 9999999967j

/ --- sv base-decode (builds 64-bit values) ---
sv[10j;1 2 3j]
sv[10;1 2 3j]
sv[256j;255 255 255 255j]
sv[24 60 60j;1 1 1j]
sv[24 60 60;1 1 1j]
sv[1000000 1000000j;5000 123456j]

/ --- vs base-encode on long (digits long; matrix; mixed radix) ---
vs[10j;123j]
vs[256j;1000j]
vs[256j;4294967295j]
vs[256;1000j]
vs[10j;1 22 333j]
vs[24 60 60j;3661j]
sv[256j;vs[256j;1099511627776j]]

/ --- dot product / matrix multiply on long ---
dot[1 2 3j;4 5 6j]
dot[1 2 3;4 5 6j]
mul[(1 2j;3 4j);(5 6j;7 8j)]
mul[(1 2;3 4);(5 6j;7 8j)]

/ --- base-conversion round-trips (long must round-trip like int) ---
/ scalar base, vector value: matrix encode then decode (was: sv type error)
10j sv vs[10j;1 22 333j]
/ vector base (mixed radix), vector value (was: vs type error)
vs[24 60 60j;3661 7322j]
24 60 60j sv vs[24 60 60j;3661 7322j]

\\
