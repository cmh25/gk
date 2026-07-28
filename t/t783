/ Sentinel-aware widening must apply to vector elements as well as atoms.
/ These are paired with their already-wide spellings: every pair must agree.

/ MC2IO (and/or/xor/div): all four i32-vector/i64 orientations.
0N 1 or 0j 0j
0Nj 1j or 0j 0j
0j 0j or 0N 1
0j 0j or 0Nj 1j
0N 1 or 0j
0Nj 1j or 0j
0j or 0N 1
0j or 0Nj 1j

/ Number theory's shared readj vector lane.
0N 6 gcd 0j 4j
0Nj 6j gcd 0j 4j
0N 3 modinv 7j 7j
0Nj 3j modinv 7j 7j

/ Integer-vector to float modulo.
0N 7!2.5
0n 7.0!2.5

/ Power's domain pre-scan must see vector sentinels as NaN/-inf, not finite
/ negative integers, before the compute path performs the same conversion.
.[{x^y};(0N 1;0.5 0.5);:]
.[{x^y};(0n 1.0;0.5 0.5);:]
.[{x^y};(0Nj 1j;0.5 0.5);:]
.[{x^y};(0n 1.0;0.5 0.5);:]
.[{x^y};(-0I 1;0.5 0.5);:]
.[{x^y};(-0i 1.0;0.5 0.5);:]

/ Float-width formatting converts its right argument to f64.
10.2$0Nj
10.2$0n
10.2$0ne
10$0N
10.2$0Ij
10.2$0i
10.2$-0Ij
10.2$-0i
\\
